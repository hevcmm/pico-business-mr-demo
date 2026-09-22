#include "RemoteXRDemoGameMode.h"

#include "RemoteXRDemoWorld.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "IXRTrackingSystem.h"

ARemoteXRDemoGameMode::ARemoteXRDemoGameMode()
{
    // The MR demo uses an explicit HMD-locked camera. GameModeBase's default
    // pawn adds an unrelated sphere at the world origin, so do not spawn it.
    DefaultPawnClass = nullptr;
}

void ARemoteXRDemoGameMode::StartPlay()
{
    Super::StartPlay();
    if (UWorld* World = GetWorld())
    {
        if (GEngine && GEngine->XRSystem.IsValid())
        {
            GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
        }
        World->SpawnActor<ARemoteXRDemoWorld>(FVector::ZeroVector, FRotator::ZeroRotator);

        // Standard UE OpenXR camera. PICO Business Streaming supplies tracking
        // and receives the compositor frames through its PC OpenXR runtime.
        if (APlayerController* Controller = World->GetFirstPlayerController())
        {
            ACameraActor* XrCamera = World->SpawnActor<ACameraActor>(
                FVector::ZeroVector, FRotator::ZeroRotator);
            if (XrCamera)
            {
                XrCamera->GetCameraComponent()->bLockToHmd = true;
                XrCamera->GetCameraComponent()->SetFieldOfView(100.0f);
                Controller->SetViewTarget(XrCamera);
                UE_LOG(LogTemp, Display, TEXT("Standard OpenXR camera is active and locked to HMD"));
            }
        }
    }
}
