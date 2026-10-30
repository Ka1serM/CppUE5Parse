#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Info.h"

#include "WorldSettings.generated.h"

namespace ue5parse::runtime {

class APlayerState;

// AWorldSettings: the settings of a world, in its persistent level: time dilation and pause (UE's
// GameFramework/WorldSettings.h).
UCLASS()
class AWorldSettings : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    float TimeDilation = 1.0f;
    UPROPERTY()
    float DemoPlayTimeDilation = 1.0f;
    UPROPERTY()
    float MinGlobalTimeDilation = 0.0001f;
    UPROPERTY()
    float MaxGlobalTimeDilation = 20.0f;
    UPROPERTY()
    APlayerState* PauserPlayerState = nullptr;

    // The dilation a world tick scales its time by.
    float GetEffectiveTimeDilation() const {
        return FMath::Clamp(TimeDilation * DemoPlayTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation);
    }
    // Clamps the dilation and returns what it became (UE's SetTimeDilation).
    float SetTimeDilation(float NewTimeDilation) {
        TimeDilation = FMath::Clamp(NewTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation);
        return TimeDilation;
    }
    APlayerState* GetPauserPlayerState() const { return PauserPlayerState; }
};

}
