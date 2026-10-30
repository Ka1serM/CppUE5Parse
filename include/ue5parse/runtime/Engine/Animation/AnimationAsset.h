#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/Engine/AnimInterpFilter.h"

#include "AnimationAsset.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;
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

// FPoseCurve: the weight of one pose of a pose asset; the pose index makes the lookup direct.
struct FPoseCurve {
    FName Name;
    int32 PoseIndex = INDEX_NONE;
    float Value = 0;

    FPoseCurve() = default;
    FPoseCurve(int32 InPoseIndex, FName InName, float InValue) : Name(InName), PoseIndex(InPoseIndex), Value(InValue) {}
};

// FAnimExtractContext: where and how to sample an animation asset (UE's Animation/AnimationAsset.h).
struct FAnimExtractContext {
    double CurrentTime = 0;
    bool bExtractRootMotion = false;
    FDeltaTimeRecord DeltaTimeRecord;
    bool bLooping = false;
    TArray<FPoseCurve> PoseCurves;
    // Per compact pose bone: the bone is sampled. A bone past the end is required.
    TArray<bool> BonesRequired;
    bool bIgnoreRootLock = false;

    FAnimExtractContext() = default;
    explicit FAnimExtractContext(double InCurrentTime, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {},
                                 bool InbLooping = false)
        : CurrentTime(InCurrentTime), bExtractRootMotion(InbExtractRootMotion), DeltaTimeRecord(InDeltaTimeRecord), bLooping(InbLooping) {}

    bool IsBoneRequired(int32 BoneIndex) const { return BoneIndex >= BonesRequired.Num() || BonesRequired[BoneIndex]; }
};

// FBlendSampleData: a blend space sample that plays this frame: its weight (and per bone weights while bones smooth
// their blend), its time and its play rate (UE's Animation/AnimationAsset.h).
USTRUCT()
struct FBlendSampleData {
    GENERATED_BODY()
    UPROPERTY()
    int32 SampleDataIndex = 0;
    UPROPERTY()
    UAnimSequence* Animation = nullptr;
    UPROPERTY()
    float TotalWeight = 0;
    UPROPERTY()
    float WeightRate = 0;
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    float PreviousTime = 0;
    UPROPERTY()
    float SamplePlayRate = 0;
    FDeltaTimeRecord DeltaTimeRecord;
    TArray<float> PerBoneBlendData;
    TArray<float> PerBoneWeightRate;

    FBlendSampleData() = default;
    FBlendSampleData(int32 Index) : SampleDataIndex(Index) {}
    bool operator==(const FBlendSampleData& Other) const { return Other.SampleDataIndex == SampleDataIndex; }
    void AddWeight(float Weight) { TotalWeight += Weight; }
    float GetClampedWeight() const { return FMath::Clamp(TotalWeight, 0.f, 1.f); }
    // Scales the weights (and each bone's weights) to a sum of 1; a list with no weight gets equal weights.
    static void NormalizeDataWeight(TArray<FBlendSampleData>& SampleDataList);
};

// FBlendFilter: the input filter of each blend space axis.
struct FBlendFilter {
    TArray<FFIRFilterTimeBased> FilterPerAxis;

    FVector GetFilterLastOutput() const {
        return FVector(FilterPerAxis.Num() > 0 ? FilterPerAxis[0].LastOutput : 0.0f, FilterPerAxis.Num() > 1 ? FilterPerAxis[1].LastOutput : 0.0f,
                       FilterPerAxis.Num() > 2 ? FilterPerAxis[2].LastOutput : 0.0f);
    }
};

namespace EAnimGroupRole {
enum Type : uint8 { CanBeLeader, AlwaysFollower, AlwaysLeader, TransitionLeader, TransitionFollower, ExclusiveAlwaysLeader };
}

enum class EAnimSyncMethod : uint8 { DoNotSync, SyncGroup, Graph };

class UAnimationAsset;

// FAnimTickRecord: an asset player's request to advance its asset this frame (UE's Animation/AnimationAsset.h). The
// update pass adds it; FAnimSync ticks it alone or in its sync group, and the asset advances TimeAccumulator.
struct FAnimTickRecord {
    const UAnimationAsset* SourceAsset = nullptr;
    float* TimeAccumulator = nullptr;
    FDeltaTimeRecord* DeltaTimeRecord = nullptr;
    float PlayRateMultiplier = 1.0f;
    float EffectiveBlendWeight = 0;
    float RootMotionWeightModifier = 1.0f;
    // The rank of the record for its group's leadership: its role, then its weight.
    float LeaderScore = 0;
    bool bLooping = false;
    bool bIsEvaluator = false;
    // A blend space player's data: its input, its samples and filter, and the triangle the last search ended in.
    struct {
        FBlendFilter* BlendFilter = nullptr;
        TArray<FBlendSampleData>* BlendSampleDataCache = nullptr;
        int32* TriangulationIndex = nullptr;
        float BlendSpacePositionX = 0;
        float BlendSpacePositionY = 0;
        bool bTeleportToTime = false;
    } BlendSpace;

    FAnimTickRecord() = default;
    FAnimTickRecord(const UAnimationAsset* InAsset, bool bInLooping, float InPlayRate, bool bInIsEvaluator, float InFinalBlendWeight,
                    float& InCurrentTime)
        : SourceAsset(InAsset), TimeAccumulator(&InCurrentTime), PlayRateMultiplier(InPlayRate), EffectiveBlendWeight(InFinalBlendWeight),
          bLooping(bInLooping), bIsEvaluator(bInIsEvaluator) {}
};

// FAnimAssetTickContext: the tick of one record or sync group: the frame's delta time and the group's position,
// which its leader sets and its followers match (UE's length-based sync; marker sync is not part of the runtime).
class FAnimAssetTickContext {
public:
    FAnimAssetTickContext(float InDeltaTime, bool bInOnlyOneAnimationInGroup)
        : DeltaTime(InDeltaTime), bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup) {}

    float GetDeltaTime() const { return DeltaTime; }
    bool IsLeader() const { return bIsLeader; }
    bool ShouldGenerateNotifies() const { return IsLeader(); }
    bool IsFollower() const { return !bIsLeader; }
    void ConvertToFollower() { bIsLeader = false; }
    bool IsSingleAnimationContext() const { return bOnlyOneAnimationInGroup; }
    void SetLeaderDelta(float InLeaderDelta) { LeaderDelta = InLeaderDelta; }
    float GetLeaderDelta() const { return LeaderDelta; }
    void SetAnimationPositionRatio(float NormalizedTime) { AnimLengthRatio = NormalizedTime; }
    float GetAnimationPositionRatio() const { return AnimLengthRatio; }
    void SetPreviousAnimationPositionRatio(float NormalizedTime) { PreviousAnimLengthRatio = NormalizedTime; }
    float GetPreviousAnimationPositionRatio() const { return PreviousAnimLengthRatio; }
    void SetResyncToSyncGroup(bool bInResyncToSyncGroup) { bResyncToSyncGroup = bInResyncToSyncGroup; }
    bool ShouldResyncToSyncGroup() const { return bResyncToSyncGroup; }

private:
    float DeltaTime = 0;
    float LeaderDelta = 0;
    float AnimLengthRatio = 0;
    float PreviousAnimLengthRatio = 0;
    bool bIsLeader = true;
    bool bOnlyOneAnimationInGroup = false;
    bool bResyncToSyncGroup = false;
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
    virtual float GetPlayLength() const { return 0; }
    // Advances the record's time: as the leader of its context by the delta time, or as a follower to the leader's
    // position.
    // A leader collects the notifies it passes into NotifyQueue.
    virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {}
    const FGuid& GetSkeletonVirtualBoneGuid() const { return SkeletonGuid; }

private:
    FGuid SkeletonGuid;
};

}
