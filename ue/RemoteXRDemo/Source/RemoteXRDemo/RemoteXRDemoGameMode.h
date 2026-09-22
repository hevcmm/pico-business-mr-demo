#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RemoteXRDemoGameMode.generated.h"

UCLASS()
class REMOTEXRDEMO_API ARemoteXRDemoGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARemoteXRDemoGameMode();
    virtual void StartPlay() override;
};
