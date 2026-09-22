#include "RemoteXRDemoWorld.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HeadMountedDisplayTypes.h"
#include "IXRTrackingSystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ARemoteXRDemoWorld::ARemoteXRDemoWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    SceneRoot->SetMobility(EComponentMobility::Static);

    UDirectionalLightComponent* Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(SceneRoot);
    Sun->SetRelativeRotation(FRotator(-45.0, -30.0, 0.0));
    Sun->SetIntensity(5.0f);
    Sun->SetAtmosphereSunLight(true);
    Sun->SetAtmosphereSunLightIndex(0);

    USkyAtmosphereComponent* SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

}

void ARemoteXRDemoWorld::BeginPlay()
{
    Super::BeginPlay();

    bMrMode = FParse::Param(FCommandLine::Get(), TEXT("PicoBusinessMR"));
    if (bMrMode)
    {
        TArray<USkyAtmosphereComponent*> SkyComponents;
        GetComponents<USkyAtmosphereComponent>(SkyComponents);
        for (USkyAtmosphereComponent* SkyComponent : SkyComponents)
        {
            SkyComponent->SetVisibility(false, true);
            SkyComponent->Deactivate();
        }

        if (FParse::Param(FCommandLine::Get(), TEXT("PicoMRTransparentProjection")))
        {
            UE_LOG(LogTemp, Display, TEXT("PICO MR transparent projection diagnostic enabled; no scene geometry"));
            return;
        }

        MovingTarget = AddShape(TEXT("MrCube"), TEXT("/Engine/BasicShapes/Cube.Cube"),
            FVector(120.0, 0.0, 0.0), FVector(0.50),
            FLinearColor(1.0f, 0.12f, 0.02f), true);
        if (MovingTarget)
        {
            MovingTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            MovingTarget->SetCastShadow(false);
        }
        UE_LOG(LogTemp, Display, TEXT("PICO Business Streaming MR scene enabled; waiting for first valid HMD pose"));
        return;
    }

    UStaticMeshComponent* SkySphere = AddShape(TEXT("SkySphere"), TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere"),
        FVector::ZeroVector, FVector(400.0), FLinearColor::White);
    if (SkySphere)
    {
        if (UMaterialInterface* SkyMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/EngineSky/M_Sky_Panning_Clouds2_Inst.M_Sky_Panning_Clouds2_Inst")))
        {
            SkySphere->SetMaterial(0, SkyMaterial);
        }
        SkySphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkySphere->SetCastShadow(false);
        UE_LOG(LogTemp, Display, TEXT("RemoteXRDemo sky sphere created with panning cloud texture material"));
    }

    AddShape(TEXT("Floor"), TEXT("/Engine/BasicShapes/Cube.Cube"),
        FVector(250.0, 0.0, -10.0), FVector(8.0, 8.0, 0.1), FLinearColor(0.08f, 0.1f, 0.13f));
    AddShape(TEXT("NearCube"), TEXT("/Engine/BasicShapes/Cube.Cube"),
        FVector(120.0, -70.0, 55.0), FVector(0.35), FLinearColor::Red, true);
    AddShape(TEXT("MidSphere"), TEXT("/Engine/BasicShapes/Sphere.Sphere"),
        FVector(250.0, 60.0, 90.0), FVector(0.55), FLinearColor::Green, true);
    AddShape(TEXT("FarCylinder"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
        FVector(430.0, -100.0, 100.0), FVector(0.55, 0.55, 1.0), FLinearColor::Blue, true);

    AddShape(TEXT("LeftEyeMarker"), TEXT("/Engine/BasicShapes/Cube.Cube"),
        FVector(180.0, -180.0, 170.0), FVector(0.2, 0.05, 0.5), FLinearColor(0.1f, 0.4f, 1.0f));
    AddShape(TEXT("RightEyeMarker"), TEXT("/Engine/BasicShapes/Cube.Cube"),
        FVector(180.0, 180.0, 170.0), FVector(0.2, 0.05, 0.5), FLinearColor(1.0f, 0.3f, 0.1f));

    for (int32 Meter = 1; Meter <= 5; ++Meter)
    {
        AddShape(*FString::Printf(TEXT("MeterMark%d"), Meter), TEXT("/Engine/BasicShapes/Cube.Cube"),
            FVector(Meter * 100.0, 0.0, 1.0), FVector(0.015, 1.5, 0.02), FLinearColor::White);
    }

    MovingTargetOrigin = FVector(300.0, 0.0, 170.0);
    MovingTarget = AddShape(TEXT("MovingTarget"), TEXT("/Engine/BasicShapes/Sphere.Sphere"),
        MovingTargetOrigin, FVector(0.3), FLinearColor::Yellow, true);
}

void ARemoteXRDemoWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ElapsedSeconds += DeltaSeconds;
    if (MovingTarget)
    {
        if (bMrMode && !bMrTargetAnchored && GEngine && GEngine->XRSystem.IsValid())
        {
            FQuat Orientation;
            FVector Position;
            if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Orientation, Position))
            {
                // PlayerCameraManager contains the final world-space HMD transform. The
                // raw XR pose above is only used to confirm that tracking is valid.
                if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
                {
                    if (APlayerCameraManager* CameraManager = Controller->PlayerCameraManager)
                    {
                        Position = CameraManager->GetCameraLocation();
                        Orientation = CameraManager->GetCameraRotation().Quaternion();
                    }
                }
                const float Yaw = Orientation.Rotator().Yaw;
                const FVector HorizontalForward = FRotator(0.0f, Yaw, 0.0f).Vector();
                MovingTargetOrigin = Position + HorizontalForward * 120.0f;
                MovingTarget->SetWorldLocation(MovingTargetOrigin);
                bMrTargetAnchored = true;
                UE_LOG(LogTemp, Display, TEXT("PICO Business MR cube anchored at %s"), *MovingTargetOrigin.ToCompactString());
            }
        }
        if (bMrMode && !bMrTargetAnchored) return;

        if (bMrMode)
        {
            MovingTarget->SetWorldRotation(FRotator(ElapsedSeconds * 18.0f, ElapsedSeconds * 28.0f, 0.0f));
            const FVector Offset(0.0f, FMath::Sin(ElapsedSeconds * 0.65f) * 18.0f,
                FMath::Sin(ElapsedSeconds * 0.45f) * 8.0f);
            MovingTarget->SetWorldLocation(MovingTargetOrigin + Offset);
            return;
        }
        const FVector Offset(0.0, FMath::Sin(ElapsedSeconds) * 100.0, FMath::Sin(ElapsedSeconds * 0.7f) * 30.0f);
        MovingTarget->SetWorldLocation(MovingTargetOrigin + Offset);
    }
}

UStaticMeshComponent* ARemoteXRDemoWorld::AddShape(const TCHAR* Name, const TCHAR* MeshPath,
    const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bMovable)
{
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Mesh || !BaseMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("RemoteXRDemo failed to load engine shape assets"));
        return nullptr;
    }

    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(Scale);
    Component->SetMobility(bMovable ? EComponentMobility::Movable : EComponentMobility::Static);
    Component->SetCollisionEnabled(bMovable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::QueryOnly);
    Component->SetCollisionProfileName(bMovable ? TEXT("PhysicsActor") : TEXT("BlockAll"));
    Component->RegisterComponent();

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Component);
    Material->SetVectorParameterValue(TEXT("Color"), Color);
    Component->SetMaterial(0, Material);
    return Component;
}
