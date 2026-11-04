#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/Engine/AnimInterpFilter.h"

#include "AnimationAsset.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;
class USkeleton;

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

struct FPoseCurve {
    FName Name;
    int32 PoseIndex = INDEX_NONE;
    float Value = 0;

    FPoseCurve() = default;
    FPoseCurve(int32 InPoseIndex, FName InName, float InValue) : Name(InName), PoseIndex(InPoseIndex), Value(InValue) {}
};

struct FAnimExtractContext {
    double CurrentTime = 0;
    bool bExtractRootMotion = false;
    FDeltaTimeRecord DeltaTimeRecord;
    bool bLooping = false;
    TArray<FPoseCurve> PoseCurves;
    TArray<bool> BonesRequired;
    bool bIgnoreRootLock = false;

    FAnimExtractContext() = default;
    explicit FAnimExtractContext(double InCurrentTime, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {},
                                 bool InbLooping = false)
        : CurrentTime(InCurrentTime), bExtractRootMotion(InbExtractRootMotion), DeltaTimeRecord(InDeltaTimeRecord), bLooping(InbLooping) {}

    bool IsBoneRequired(int32 BoneIndex) const { return BoneIndex >= BonesRequired.Num() || BonesRequired[BoneIndex]; }
};

USTRUCT()
struct FBlendSampleData {
    GENERATED_BODY()
    UPROPERTY()
    int32 SampleDataIndex = 0;
    UPROPERTY()
    TObjectPtr<UAnimSequence> Animation = nullptr;
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
    static void NormalizeDataWeight(TArray<FBlendSampleData>& SampleDataList);
};

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

struct FAnimTickRecord {
    const UAnimationAsset* SourceAsset = nullptr;
    float* TimeAccumulator = nullptr;
    FDeltaTimeRecord* DeltaTimeRecord = nullptr;
    float PlayRateMultiplier = 1.0f;
    float EffectiveBlendWeight = 0;
    float RootMotionWeightModifier = 1.0f;
    float LeaderScore = 0;
    bool bLooping = false;
    bool bIsEvaluator = false;
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

class FAnimAssetTickContext {
public:
    FAnimAssetTickContext(float InDeltaTime, bool bInOnlyOneAnimationInGroup)
        : DeltaTime(InDeltaTime), bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup) {}

    float GetDeltaTime() const { return DeltaTime; }
    bool IsLeader() const { return bIsLeader; }
    bool ShouldGenerateNotifies() const { return IsLeader(); }
    void ConvertToFollower() { bIsLeader = false; }
    void SetLeaderDelta(float InLeaderDelta) { LeaderDelta = InLeaderDelta; }
    void SetAnimationPositionRatio(float NormalizedTime) { AnimLengthRatio = NormalizedTime; }
    float GetAnimationPositionRatio() const { return AnimLengthRatio; }
    void SetPreviousAnimationPositionRatio(float NormalizedTime) { PreviousAnimLengthRatio = NormalizedTime; }
    float GetPreviousAnimationPositionRatio() const { return PreviousAnimLengthRatio; }
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

UCLASS(Abstract)
class UAnimationAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> Skeleton = nullptr;

    void Serialize(FArchive& Ar) override;
    USkeleton* GetSkeleton() const { return Skeleton; }
    virtual float GetPlayLength() const { return 0; }
    virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {}

private:
    FGuid SkeletonGuid;
};

}
