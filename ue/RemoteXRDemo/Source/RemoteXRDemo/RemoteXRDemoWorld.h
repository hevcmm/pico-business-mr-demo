#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemoteXRDemoWorld.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class REMOTEXRDEMO_API ARemoteXRDemoWorld final : public AActor
{
    GENERATED_BODY()

public:
    ARemoteXRDemoWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UStaticMeshComponent* AddShape(const TCHAR* Name, const TCHAR* MeshPath,
        const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bMovable = false);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> MovingTarget;

    FVector MovingTargetOrigin = FVector::ZeroVector;
    float ElapsedSeconds = 0.0f;
    bool bMrMode = false;
    bool bMrTargetAnchored = false;
};
