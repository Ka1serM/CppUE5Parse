#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "AnimNode_BlendSpacePlayer.generated.h"

namespace ue5parse::runtime {

// Plays a blend space at an input position (UE's AnimNodes/AnimNode_BlendSpacePlayer.h). The base has the logic; the
// subclasses say where the inputs are (folded or not). The time is normalized: 0 to 1 over the blended length.
USTRUCT()
struct FAnimNode_BlendSpacePlayerBase : public FAnimNode_AssetPlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    UBlendSpace* PreviousBlendSpace = nullptr;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    float GetCurrentAssetTime() const override;
    float GetCurrentAssetTimePlayRateAdjusted() const override;
    float GetCurrentAssetLength() const override;
    UAnimationAsset* GetAnimAsset() const override { return GetBlendSpace(); }
    float GetTimeFromEnd(float CurrentTime) const { return GetBlendSpace() != nullptr ? 1.0f - CurrentTime : 0.0f; }
    FVector GetFilteredPosition() const { return BlendFilter.GetFilterLastOutput(); }
    // Sets the filters to a position, so that the input does not smooth toward it.
    void SnapToPosition(const FVector& NewPosition);

    virtual UBlendSpace* GetBlendSpace() const { return nullptr; }
    virtual FVector GetPosition() const { return FVector::ZeroVector; }
    virtual float GetStartPosition() const { return 0.0f; }
    virtual float GetPlayRate() const { return 1.0f; }
    virtual bool IsLooping() const { return true; }
    virtual bool ShouldResetPlayTimeWhenBlendSpaceChanges() const { return true; }
    virtual bool ShouldTeleportToTime() const { return false; }
    virtual bool IsEvaluator() const { return false; }

protected:
    // The tick record of this frame, for the instance's sync.
    void UpdateInternal(const FAnimationUpdateContext& Context);

    FBlendFilter BlendFilter;
    TArray<FBlendSampleData> BlendSampleDataCache;
    int32 CachedTriangulationIndex = -1;

private:
    void Reinitialize(bool bResetTime = true);
    const FBlendSampleData* GetHighestWeightedSample() const;
};

// The player whose inputs the class folds.
USTRUCT()
struct FAnimNode_BlendSpacePlayer : public FAnimNode_BlendSpacePlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    UBlendSpace* BlendSpace = nullptr;
    UPROPERTY()
    bool bReplicate = false;

    FName GetGroupName() const override { return GET_ANIM_NODE_DATA(FName, GroupName); }
    EAnimGroupRole::Type GetGroupRole() const override { return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole); }
    EAnimSyncMethod GetGroupMethod() const override { return GET_ANIM_NODE_DATA(EAnimSyncMethod, Method); }
    bool GetIgnoreForRelevancyTest() const override { return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest); }
    bool IsLooping() const override { return GET_ANIM_NODE_DATA(bool, bLoop); }
    UBlendSpace* GetBlendSpace() const override { return BlendSpace; }
    FVector GetPosition() const override { return FVector(GET_ANIM_NODE_DATA(float, X), GET_ANIM_NODE_DATA(float, Y), 0.0f); }
    float GetStartPosition() const override { return GET_ANIM_NODE_DATA(float, StartPosition); }
    float GetPlayRate() const override { return GET_ANIM_NODE_DATA(float, PlayRate); }
    bool ShouldResetPlayTimeWhenBlendSpaceChanges() const override { return GET_ANIM_NODE_DATA(bool, bResetPlayTimeWhenBlendSpaceChanges); }
};

// The player that holds its inputs itself (made by code, not by a Blueprint).
USTRUCT()
struct FAnimNode_BlendSpacePlayer_Standalone : public FAnimNode_BlendSpacePlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName GroupName;
    UPROPERTY()
    TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;
    UPROPERTY()
    EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;
    UPROPERTY()
    bool bIgnoreForRelevancyTest = false;
    UPROPERTY()
    float X = 0.0f;
    UPROPERTY()
    float Y = 0.0f;
    UPROPERTY()
    float PlayRate = 1.0f;
    UPROPERTY()
    bool bLoop = true;
    UPROPERTY()
    bool bResetPlayTimeWhenBlendSpaceChanges = true;
    UPROPERTY()
    float StartPosition = 0.0f;
    UPROPERTY()
    UBlendSpace* BlendSpace = nullptr;

    FName GetGroupName() const override { return GroupName; }
    EAnimGroupRole::Type GetGroupRole() const override { return GroupRole; }
    EAnimSyncMethod GetGroupMethod() const override { return Method; }
    bool GetIgnoreForRelevancyTest() const override { return bIgnoreForRelevancyTest; }
    bool IsLooping() const override { return bLoop; }
    UBlendSpace* GetBlendSpace() const override { return BlendSpace; }
    FVector GetPosition() const override { return FVector(X, Y, 0.0); }
    float GetStartPosition() const override { return StartPosition; }
    float GetPlayRate() const override { return PlayRate; }
    bool ShouldResetPlayTimeWhenBlendSpaceChanges() const override { return bResetPlayTimeWhenBlendSpaceChanges; }
};

}
