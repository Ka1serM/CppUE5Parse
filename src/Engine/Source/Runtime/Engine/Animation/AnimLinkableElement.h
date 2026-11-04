#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AnimLinkableElement.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UAnimSequenceBase;

namespace EAnimLinkMethod {

enum Type : int32 { Absolute, Relative, Proportional };

}

USTRUCT()
struct FAnimLinkableElement {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAnimMontage> LinkedMontage = nullptr;
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
    TObjectPtr<UAnimSequenceBase> LinkedSequence = nullptr;

    float GetTime(EAnimLinkMethod::Type ReferenceFrame = EAnimLinkMethod::Absolute) const;
};

}
