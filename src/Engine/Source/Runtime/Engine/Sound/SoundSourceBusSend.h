#pragma once

#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "SoundSourceBusSend.generated.h"

namespace ue5parse::runtime {

class UAudioBus;
class USoundSourceBus;

UENUM()
enum class ESourceBusSendLevelControlMethod : uint8 {
    Linear,
    CustomCurve,
    Manual,
};

USTRUCT()
struct FSoundSourceBusSendInfo {
    GENERATED_BODY()
    UPROPERTY()
    ESourceBusSendLevelControlMethod SourceBusSendLevelControlMethod = ESourceBusSendLevelControlMethod::Manual;
    UPROPERTY()
    TObjectPtr<USoundSourceBus> SoundSourceBus = nullptr;
    UPROPERTY()
    TObjectPtr<UAudioBus> AudioBus = nullptr;
    UPROPERTY()
    float SendLevel = 1.0f;
    UPROPERTY()
    float MinSendLevel = 0.0f;
    UPROPERTY()
    float MaxSendLevel = 1.0f;
    UPROPERTY()
    float MinSendDistance = 100.0f;
    UPROPERTY()
    float MaxSendDistance = 1000.0f;
    UPROPERTY()
    FRuntimeFloatCurve CustomSendLevelCurve;
    UPROPERTY()
    uint8 bEnableLPFCutoff : 1 = 0;
    UPROPERTY()
    float LPFCutoff = 20000.0f;
    UPROPERTY()
    uint8 bEnableHPFCutoff : 1 = 0;
    UPROPERTY()
    float HPFCutoff = 20.0f;
    UPROPERTY()
    TOptional<float> LPFCutoffFrequency_DEPRECATED;
    UPROPERTY()
    TOptional<float> HPFCutoffFrequency_DEPRECATED;
};

}
