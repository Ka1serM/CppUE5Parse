#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"

#include "AnimNode_SequenceEvaluator.generated.h"

namespace ue5parse::runtime {

class UAnimSequenceBase;

namespace ESequenceEvalReinit {
enum Type : int32 { NoReset, StartPosition, ExplicitTime };
}

// Evaluates a sequence at an input time (UE's AnimNodes/AnimNode_SequenceEvaluator.h). A node in a sync group, or one
// that does not teleport, advances to the time; else it jumps there.
USTRUCT()
struct FAnimNode_SequenceEvaluatorBase : public FAnimNode_AssetPlayerBase {
    GENERATED_BODY()
public:
    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    float GetCurrentAssetTime() const override { return GetExplicitTime(); }
    float GetCurrentAssetLength() const override;
    float GetAccumulatedTime() const override { return GetExplicitTime(); }
    UAnimationAsset* GetAnimAsset() const override;
    // The move from the previous time to the input time; a looping sequence takes the shorter way around.
    float GetEffectiveDeltaTime(float ExplicitTime, float PrevExplicitTime) const;

    virtual UAnimSequenceBase* GetSequence() const { return nullptr; }
    virtual float GetExplicitTime() const { return 0.0f; }
    virtual bool IsLooping() const { return true; }
    virtual bool GetTeleportToExplicitTime() const { return true; }
    virtual ESequenceEvalReinit::Type GetReinitializationBehavior() const { return ESequenceEvalReinit::ExplicitTime; }
    virtual float GetStartPosition() const { return 0.0f; }

private:
    bool bReinitialized = false;
};

USTRUCT()
struct FAnimNode_SequenceEvaluator : public FAnimNode_SequenceEvaluatorBase {
    GENERATED_BODY()
public:
    UAnimSequenceBase* GetSequence() const override { return GET_ANIM_NODE_DATA(UAnimSequenceBase*, Sequence); }
    float GetExplicitTime() const override { return GET_ANIM_NODE_DATA(float, ExplicitTime); }
    bool IsLooping() const override { return GET_ANIM_NODE_DATA(bool, bShouldLoop); }
    bool GetTeleportToExplicitTime() const override { return GET_ANIM_NODE_DATA(bool, bTeleportToExplicitTime); }
    ESequenceEvalReinit::Type GetReinitializationBehavior() const override {
        return GET_ANIM_NODE_DATA(TEnumAsByte<ESequenceEvalReinit::Type>, ReinitializationBehavior);
    }
    float GetStartPosition() const override { return GET_ANIM_NODE_DATA(float, StartPosition); }
    FName GetGroupName() const override { return GET_ANIM_NODE_DATA(FName, GroupName); }
    EAnimGroupRole::Type GetGroupRole() const override { return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole); }
    EAnimSyncMethod GetGroupMethod() const override { return GET_ANIM_NODE_DATA(EAnimSyncMethod, Method); }
    bool GetIgnoreForRelevancyTest() const override { return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest); }
};

USTRUCT()
struct FAnimNode_SequenceEvaluator_Standalone : public FAnimNode_SequenceEvaluatorBase {
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
    UAnimSequenceBase* Sequence = nullptr;
    UPROPERTY()
    float ExplicitTime = 0.0f;
    UPROPERTY()
    bool bShouldLoop = true;
    UPROPERTY()
    bool bTeleportToExplicitTime = true;
    UPROPERTY()
    TEnumAsByte<ESequenceEvalReinit::Type> ReinitializationBehavior = ESequenceEvalReinit::ExplicitTime;
    UPROPERTY()
    float StartPosition = 0.0f;

    UAnimSequenceBase* GetSequence() const override { return Sequence; }
    float GetExplicitTime() const override { return ExplicitTime; }
    bool IsLooping() const override { return bShouldLoop; }
    bool GetTeleportToExplicitTime() const override { return bTeleportToExplicitTime; }
    ESequenceEvalReinit::Type GetReinitializationBehavior() const override { return ReinitializationBehavior; }
    float GetStartPosition() const override { return StartPosition; }
    FName GetGroupName() const override { return GroupName; }
    EAnimGroupRole::Type GetGroupRole() const override { return GroupRole; }
    EAnimSyncMethod GetGroupMethod() const override { return Method; }
    bool GetIgnoreForRelevancyTest() const override { return bIgnoreForRelevancyTest; }
};

}
