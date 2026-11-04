#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "WorldSettings.generated.h"

namespace ue5parse::runtime {

class APlayerState;

UCLASS()
class AWorldSettings : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY(Replicated)
    float TimeDilation = 1.0f;
    UPROPERTY()
    float DemoPlayTimeDilation = 1.0f;
    UPROPERTY()
    float MinGlobalTimeDilation = 0.0001f;
    UPROPERTY()
    float MaxGlobalTimeDilation = 20.0f;
    UPROPERTY(Replicated)
    TObjectPtr<APlayerState> PauserPlayerState = nullptr;

    float GetEffectiveTimeDilation() const {
        return FMath::Clamp(TimeDilation * DemoPlayTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation);
    }
    float SetTimeDilation(float NewTimeDilation) {
        TimeDilation = FMath::Clamp(NewTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation);
        return TimeDilation;
    }
    APlayerState* GetPauserPlayerState() const { return PauserPlayerState; }
};

}
