#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "AnimNode_BlendSpaceGraphBase.generated.h"

namespace ue5parse::runtime {

// A blend space whose samples are graphs: sample i is the pose of SamplePoseLinks[i] (UE's
// AnimNodes/AnimNode_BlendSpaceGraphBase.h). The sync group scope that UE gives the sample graphs is not part of the
// runtime: their players keep their own groups.
USTRUCT()
struct FAnimNode_BlendSpaceGraphBase : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    float X = 0.0f;
    UPROPERTY()
    float Y = 0.0f;
    UPROPERTY()
    FName GroupName;
    UPROPERTY()
    TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;
    UPROPERTY()
    const UBlendSpace* BlendSpace = nullptr;
    UPROPERTY()
    TArray<FPoseLink> SamplePoseLinks;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    const UBlendSpace* GetBlendSpace() const { return BlendSpace; }
    FVector GetPosition() const { return FVector(X, Y, 0); }
    FVector GetFilteredPosition() const { return BlendFilter.GetFilterLastOutput(); }
    void SnapToPosition(const FVector& NewPosition);

protected:
    void UpdateInternal(const FAnimationUpdateContext& Context);

    FBlendFilter BlendFilter;
    TArray<FBlendSampleData> BlendSampleDataCache;
    int32 CachedTriangulationIndex = -1;
};

USTRUCT()
struct FAnimNode_BlendSpaceGraph : public FAnimNode_BlendSpaceGraphBase {
    GENERATED_BODY()
public:
    FName GetGroupName() const { return GroupName; }
};

}
