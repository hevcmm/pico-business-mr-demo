#include "IOpenXRExtensionPlugin.h"
#include "OpenXRCore.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY_STATIC(LogPicoBusinessMR, Log, All);

class FPicoBusinessMRModule final : public IModuleInterface, public IOpenXRExtensionPlugin
{
public:
    virtual void StartupModule() override
    {
        bEnabled = FParse::Param(FCommandLine::Get(), TEXT("PicoBusinessMR"));
        bPassthroughOnly = FParse::Param(FCommandLine::Get(), TEXT("PicoMRPassthroughOnly"));
        bEnvironmentBlendOnly = FParse::Param(FCommandLine::Get(), TEXT("PicoMREnvironmentBlendOnly"));
        IModularFeatures::Get().RegisterModularFeature(
            IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
        UE_LOG(LogPicoBusinessMR, Display, TEXT("PICO Business MR extension registered (enabled=%s)"),
            bEnabled ? TEXT("true") : TEXT("false"));
    }

    virtual void ShutdownModule() override
    {
        IModularFeatures::Get().UnregisterModularFeature(
            IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
    }

    virtual FString GetDisplayName() override { return TEXT("PICO Business MR Passthrough"); }

    virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override
    {
        if (bEnabled && !bEnvironmentBlendOnly)
        {
            OutExtensions.Add(XR_FB_PASSTHROUGH_EXTENSION_NAME);
        }
        return true;
    }

    virtual void PostCreateInstance(XrInstance InInstance) override
    {
        Instance = InInstance;
        if (!bEnabled || bEnvironmentBlendOnly) return;
        xrGetInstanceProcAddr(Instance, "xrCreatePassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&CreatePassthrough));
        xrGetInstanceProcAddr(Instance, "xrDestroyPassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&DestroyPassthrough));
        xrGetInstanceProcAddr(Instance, "xrCreatePassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&CreateLayer));
        xrGetInstanceProcAddr(Instance, "xrDestroyPassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&DestroyLayer));
    }

    virtual void PostCreateSession(XrSession InSession) override
    {
        if (!bEnabled || bEnvironmentBlendOnly)
        {
            UE_LOG(LogPicoBusinessMR, Display,
                TEXT("Using XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND without XR_FB_passthrough"));
            return;
        }
        if (!CreatePassthrough || !CreateLayer)
        {
            UE_LOG(LogPicoBusinessMR, Error, TEXT("XR_FB_passthrough entry points are unavailable"));
            return;
        }

        CreatePassthroughSet(InSession, TEXT("session-create"));
    }

    virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override
    {
        if (!bEnabled || !InHeader || InHeader->type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
        {
            return;
        }

        const XrEventDataSessionStateChanged* StateEvent =
            reinterpret_cast<const XrEventDataSessionStateChanged*>(InHeader);
        UE_LOG(LogPicoBusinessMR, Display, TEXT("OpenXR session state=%d"),
            static_cast<int32>(StateEvent->state));

        if (StateEvent->state == XR_SESSION_STATE_READY)
        {
            if (bSeenReadyEvent)
            {
                // PICO Business Streaming keeps the PC OpenXR session alive across
                // a headset reconnect, but the headset drops the remote passthrough
                // handle. Publish a fresh running layer on the next READY event.
                CreatePassthroughSet(InSession, TEXT("session-ready-reconnect"));
            }
            bSeenReadyEvent = true;
        }
    }

    void CreatePassthroughSet(XrSession InSession, const TCHAR* Reason)
    {
        XrPassthroughFB NewPassthrough = XR_NULL_HANDLE;
        XrPassthroughLayerFB NewLayer = XR_NULL_HANDLE;

        XrPassthroughCreateInfoFB PassthroughInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        PassthroughInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
        XrResult Result = CreatePassthrough(InSession, &PassthroughInfo, &NewPassthrough);
        if (XR_FAILED(Result))
        {
            UE_LOG(LogPicoBusinessMR, Error, TEXT("Passthrough create failed (%s): %d"),
                Reason, static_cast<int32>(Result));
            return;
        }

        XrPassthroughLayerCreateInfoFB LayerInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
        LayerInfo.passthrough = NewPassthrough;
        LayerInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
        LayerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
        Result = CreateLayer(InSession, &LayerInfo, &NewLayer);
        if (XR_FAILED(Result))
        {
            UE_LOG(LogPicoBusinessMR, Error, TEXT("Passthrough layer create failed (%s): %d"),
                Reason, static_cast<int32>(Result));
            if (DestroyPassthrough) DestroyPassthrough(NewPassthrough);
            return;
        }

        {
            FScopeLock Lock(&PassthroughLock);
            Passthrough = NewPassthrough;
            PassthroughLayer = NewLayer;
            CompositionLayer = {XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
            CompositionLayer.layerHandle = NewLayer;
        }
        UE_LOG(LogPicoBusinessMR, Display, TEXT("PICO passthrough layer published (%s)"), Reason);
    }

    virtual void OnDestroySession(XrSession) override
    {
        FScopeLock Lock(&PassthroughLock);
        if (PassthroughLayer != XR_NULL_HANDLE && DestroyLayer) DestroyLayer(PassthroughLayer);
        if (Passthrough != XR_NULL_HANDLE && DestroyPassthrough) DestroyPassthrough(Passthrough);
        PassthroughLayer = XR_NULL_HANDLE;
        Passthrough = XR_NULL_HANDLE;
    }

    virtual void UpdateCompositionLayers_RHIThread(
        XrSession, TArray<XrCompositionLayerBaseHeader*>& Headers) override
    {
        FScopeLock Lock(&PassthroughLock);
        if (bEnabled && !bEnvironmentBlendOnly && PassthroughLayer != XR_NULL_HANDLE)
        {
            if (bPassthroughOnly)
            {
                Headers.Reset();
            }
            // OpenXR layers are ordered back-to-front. Passthrough must be the
            // background (index 0), with the alpha-enabled projection above it.
            Headers.Insert(reinterpret_cast<XrCompositionLayerBaseHeader*>(&CompositionLayer), 0);
            if (!bLoggedLayerOrder)
            {
                bLoggedLayerOrder = true;
                UE_LOG(LogPicoBusinessMR, Display,
                    TEXT("Submitting passthrough at layer 0 behind %d other composition layer(s)"),
                    Headers.Num() - 1);
            }
        }
    }

    virtual const void* OnEndProjectionLayer_RHIThread(
        XrSession, int32, const void* InNext, XrCompositionLayerFlags& OutFlags) override
    {
        if (bEnabled)
        {
            // PICO Business Streaming 2.2 drops the right-eye projection when
            // UNPREMULTIPLIED_ALPHA is present. UE's full-texture alpha pass
            // already prepares both eye regions, so source alpha is sufficient.
            OutFlags |= XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            if (!bLoggedProjectionFlags)
            {
                bLoggedProjectionFlags = true;
                UE_LOG(LogPicoBusinessMR, Display,
                    TEXT("Projection alpha flags=0x%llx (layer=%d)"),
                    static_cast<unsigned long long>(OutFlags), 0);
            }
        }
        return InNext;
    }

private:
    bool bEnabled = false;
    bool bPassthroughOnly = false;
    bool bEnvironmentBlendOnly = false;
    bool bLoggedLayerOrder = false;
    bool bLoggedProjectionFlags = false;
    bool bSeenReadyEvent = false;
    FCriticalSection PassthroughLock;
    XrInstance Instance = XR_NULL_HANDLE;
    XrPassthroughFB Passthrough = XR_NULL_HANDLE;
    XrPassthroughLayerFB PassthroughLayer = XR_NULL_HANDLE;
    XrCompositionLayerPassthroughFB CompositionLayer{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
    PFN_xrCreatePassthroughFB CreatePassthrough = nullptr;
    PFN_xrDestroyPassthroughFB DestroyPassthrough = nullptr;
    PFN_xrCreatePassthroughLayerFB CreateLayer = nullptr;
    PFN_xrDestroyPassthroughLayerFB DestroyLayer = nullptr;
};

IMPLEMENT_MODULE(FPicoBusinessMRModule, PicoBusinessMR)
