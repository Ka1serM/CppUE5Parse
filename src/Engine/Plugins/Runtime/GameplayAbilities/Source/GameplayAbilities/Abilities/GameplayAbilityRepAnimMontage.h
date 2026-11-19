#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"

#include "GameplayAbilityRepAnimMontage.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UAnimSequenceBase;

USTRUCT()
struct FGameplayAbilityRepAnimMontage {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAnimMontage> AnimMontage_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimSequenceBase> Animation = nullptr;
    UPROPERTY()
    FName SlotName;
    UPROPERTY()
    float PlayRate = 0;
    UPROPERTY()
    float Position = 0;
    UPROPERTY()
    float BlendTime = 0;
    UPROPERTY()
    float BlendOutTime = 0;
    UPROPERTY()
    float PlayCount = 0;
    UPROPERTY()
    uint8 NextSectionID = 0;
    UPROPERTY()
    uint8 PlayInstanceId = 0;
    UPROPERTY()
    uint8 bRepPosition : 1 = 0;
    UPROPERTY()
    uint8 IsStopped : 1 = 0;
    UPROPERTY()
    uint8 ForcePlayBit : 1 = 0;
    UPROPERTY()
    uint8 SkipPositionCorrection : 1 = 0;
    UPROPERTY()
    uint8 bSkipPlayRate : 1 = 0;
    UPROPERTY()
    FPredictionKey PredictionKey;
    UPROPERTY()
    uint8 SectionIdToPlay = 0;
};

}
