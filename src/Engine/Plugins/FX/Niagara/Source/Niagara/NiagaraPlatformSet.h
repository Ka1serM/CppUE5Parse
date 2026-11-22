#pragma once


#include "NiagaraPlatformSet.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraDeviceProfileStateEntry {
    GENERATED_BODY()

    UPROPERTY()
    FName ProfileName;
    UPROPERTY()
    uint32 QualityLevelMask = 0;
    UPROPERTY()
    uint32 SetQualityLevelMask = 0;
};

UENUM()
enum class ENiagaraCVarConditionResponse : uint8 { None, Enable, Disable };

USTRUCT()
struct FNiagaraPlatformSetCVarCondition {
    GENERATED_BODY()

    UPROPERTY()
    FName CVarName;
    UPROPERTY()
    bool Value = true;
    UPROPERTY()
    int32 MinInt = 1;
    UPROPERTY()
    int32 MaxInt = 1;
    UPROPERTY()
    float MinFloat = 1.0f;
    UPROPERTY()
    float MaxFloat = 1.0f;
    UPROPERTY()
    uint32 bUseMinInt : 1 = 0;
    UPROPERTY()
    uint32 bUseMaxInt : 1 = 0;
    UPROPERTY()
    uint32 bUseMinFloat : 1 = 0;
    UPROPERTY()
    uint32 bUseMaxFloat : 1 = 0;
    UPROPERTY()
    ENiagaraCVarConditionResponse PassResponse = ENiagaraCVarConditionResponse::None;
    UPROPERTY()
    ENiagaraCVarConditionResponse FailResponse = ENiagaraCVarConditionResponse::Disable;
};

USTRUCT()
struct FNiagaraPlatformSet {
    GENERATED_BODY()

    UPROPERTY()
    int32 QualityLevelMask = 0;
    UPROPERTY()
    TArray<FNiagaraDeviceProfileStateEntry> DeviceProfileStates;
    UPROPERTY()
    TArray<FNiagaraPlatformSetCVarCondition> CVarConditions;
};

}
