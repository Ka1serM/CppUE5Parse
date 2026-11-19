#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/AttributeSet.h"

#include "GameplayEffectAttributeCaptureDefinition.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EGameplayEffectAttributeCaptureSource : uint8 { Source, Target };

USTRUCT()
struct FGameplayEffectAttributeCaptureDefinition {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayAttribute AttributeToCapture;
    UPROPERTY()
    EGameplayEffectAttributeCaptureSource AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
    UPROPERTY()
    bool bSnapshot = false;
};

}
