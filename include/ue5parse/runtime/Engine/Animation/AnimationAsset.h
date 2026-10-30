#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AnimationAsset.generated.h"

namespace ue5parse::runtime {

class USkeleton;

// FDeltaTimeRecord: the previous position and the move of an asset player this update.
struct FDeltaTimeRecord {
    float Delta = 0;
    float Previous = 0;
    bool bPreviousIsValid = false;

    void Set(float InPrevious, float InDelta) {
        Previous = InPrevious;
        Delta = InDelta;
        bPreviousIsValid = true;
    }
    void SetPrevious(float InPrevious) {
        Previous = InPrevious;
        bPreviousIsValid = true;
    }
    bool IsPreviousValid() const { return bPreviousIsValid; }
};

// FAnimExtractContext: where and how to sample an animation asset (UE's Animation/AnimationAsset.h).
struct FAnimExtractContext {
    double CurrentTime = 0;
    bool bExtractRootMotion = false;
    FDeltaTimeRecord DeltaTimeRecord;
    bool bLooping = false;
    // Pose asset weights: the pose name, its weight.
    TArray<std::pair<FName, float>> PoseCurves;
    bool bIgnoreRootLock = false;

    FAnimExtractContext() = default;
    explicit FAnimExtractContext(double InCurrentTime, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {},
                                 bool InbLooping = false)
        : CurrentTime(InCurrentTime), bExtractRootMotion(InbExtractRootMotion), DeltaTimeRecord(InDeltaTimeRecord), bLooping(InbLooping) {}
};

// UAnimationAsset (UE's Animation/AnimationAsset.h): an asset made for a skeleton.
UCLASS(Abstract)
class UAnimationAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    USkeleton* Skeleton = nullptr;

    void Serialize(FArchive& Ar) override;
    USkeleton* GetSkeleton() const { return Skeleton; }
    const FGuid& GetSkeletonVirtualBoneGuid() const { return SkeletonGuid; }

private:
    FGuid SkeletonGuid;
};

}
