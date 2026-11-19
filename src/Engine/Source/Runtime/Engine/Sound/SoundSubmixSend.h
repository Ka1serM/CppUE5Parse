#pragma once

#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "SoundSubmixSend.generated.h"

namespace ue5parse::runtime {

class USoundSubmixBase;

UENUM()
enum class ESendLevelControlMethod : uint8 {
    Linear,
    CustomCurve,
    Manual,
};

UENUM()
enum class ESubmixSendStage : uint8 {
    PostDistanceAttenuation,
    PreDistanceAttenuation,
};

USTRUCT()
struct FSoundSubmixSendInfoBase {
    GENERATED_BODY()
    UPROPERTY()
    ESendLevelControlMethod SendLevelControlMethod = ESendLevelControlMethod::Manual;
    UPROPERTY()
    TObjectPtr<USoundSubmixBase> SoundSubmix = nullptr;
    UPROPERTY()
    float SendLevel = 0.0f;
    UPROPERTY()
    bool DisableManualSendClamp = false;
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

USTRUCT()
struct FSoundSubmixSendInfo : public FSoundSubmixSendInfoBase {
    GENERATED_BODY()
    UPROPERTY()
    ESubmixSendStage SendStage = ESubmixSendStage::PostDistanceAttenuation;
};

}
