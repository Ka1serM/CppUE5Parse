#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AnimLinkableElement.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UAnimSequenceBase;

namespace EAnimLinkMethod {
enum Type : int32 { Absolute, Relative, Proportional };
}

// FAnimLinkableElement: a time that can be linked to a montage segment (UE's Animation/AnimLinkableElement.h).
USTRUCT()
struct FAnimLinkableElement {
    GENERATED_BODY()
    UPROPERTY()
    UAnimMontage* LinkedMontage = nullptr;
    UPROPERTY()
    int32 SlotIndex = 0;
    UPROPERTY()
    int32 SegmentIndex = INDEX_NONE;
    UPROPERTY()
    TEnumAsByte<EAnimLinkMethod::Type> LinkMethod = EAnimLinkMethod::Absolute;
    UPROPERTY()
    TEnumAsByte<EAnimLinkMethod::Type> CachedLinkMethod = EAnimLinkMethod::Absolute;
    UPROPERTY()
    float SegmentBeginTime = 0;
    UPROPERTY()
    float SegmentLength = 0;
    UPROPERTY()
    float LinkValue = 0;
    UPROPERTY()
    UAnimSequenceBase* LinkedSequence = nullptr;

    // The absolute time in the asset, from the link value and the method.
    float GetTime(EAnimLinkMethod::Type ReferenceFrame = EAnimLinkMethod::Absolute) const;
    float GetLinkValue() const { return LinkValue; }
};

}
