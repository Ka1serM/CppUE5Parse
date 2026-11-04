#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "GameStateBase.generated.h"

namespace ue5parse::runtime {

class APlayerState;

UCLASS()
class AGameStateBase : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<APlayerState>> PlayerArray;

    void PostInitializeComponents() override;
};

}
