#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimSequenceBase.h"

#include "AnimCompositeBase.generated.h"

namespace ue5parse::runtime {

// FAnimSegment: a range of an animation placed on a track (UE's Animation/AnimCompositeBase.h).
USTRUCT()
struct FAnimSegment {
    GENERATED_BODY()
    UPROPERTY()
    UAnimSequenceBase* AnimReference = nullptr;
    UPROPERTY()
    float StartPos = 0;
    UPROPERTY()
    float AnimStartTime = 0;
    UPROPERTY()
    float AnimEndTime = 0;
    UPROPERTY()
    float AnimPlayRate = 1;
    UPROPERTY()
    int32 LoopingCount = 1;

    UAnimSequenceBase* GetAnimReference() const { return AnimReference; }
    float GetLength() const;
    float GetEndPos() const { return StartPos + GetLength(); }
};

USTRUCT()
struct FAnimTrack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAnimSegment> AnimSegments;

    float GetLength() const;
};

// UAnimCompositeBase: an asset made of segments of other animations.
UCLASS(Abstract)
class UAnimCompositeBase : public UAnimSequenceBase {
    GENERATED_BODY()
};

}
