#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "AnimInstanceLocatorFragment.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EAnimInstanceLocatorFragmentType : int32 { AnimInstance, PostProcessAnimInstance };

USTRUCT()
struct FAnimInstanceLocatorFragment {
    GENERATED_BODY()

    UPROPERTY()
    EAnimInstanceLocatorFragmentType Type = EAnimInstanceLocatorFragmentType::AnimInstance;
};

}
