#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_SequencePlayer.generated.h"

namespace ue5parse::runtime {

class UAnimSequenceBase;

// Plays a sequence (UE's Animation/AnimNode_SequencePlayer.h). The base has the logic; the subclass says where the
// inputs are (folded or not).
USTRUCT()
struct FAnimNode_SequencePlayerBase : public FAnimNode_AssetPlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FInputScaleBiasClampState PlayRateScaleBiasClampState;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    UAnimationAsset* GetAnimAsset() const override;
    float GetCurrentAssetTime() const override { return InternalTimeAccumulator; }
    float GetCurrentAssetTimePlayRateAdjusted() const override;
    float GetCurrentAssetLength() const override;

    virtual UAnimSequenceBase* GetSequence() const { return nullptr; }
    virtual float GetPlayRateBasis() const { return 1.0f; }
    virtual float GetPlayRate() const { return 1.0f; }
    virtual const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const;
    virtual float GetStartPosition() const { return 0; }
    virtual bool IsLooping() const { return true; }

private:
    float GetAdjustedPlayRate(float DeltaTime);
};

USTRUCT()
struct FAnimNode_SequencePlayer : public FAnimNode_SequencePlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UAnimSequenceBase> Sequence = nullptr;

    UAnimSequenceBase* GetSequence() const override { return Sequence; }
    float GetPlayRateBasis() const override { return GET_ANIM_NODE_DATA(float, PlayRateBasis); }
    float GetPlayRate() const override { return GET_ANIM_NODE_DATA(float, PlayRate); }
    const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const override {
        return GET_ANIM_NODE_DATA(FInputScaleBiasClampConstants, PlayRateScaleBiasClampConstants);
    }
    float GetStartPosition() const override { return GET_ANIM_NODE_DATA(float, StartPosition); }
    bool IsLooping() const override { return GET_ANIM_NODE_DATA(bool, bLoopAnimation); }
    FName GetGroupName() const override { return GET_ANIM_NODE_DATA(FName, GroupName); }
    EAnimGroupRole::Type GetGroupRole() const override { return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole); }
    EAnimSyncMethod GetGroupMethod() const override { return GET_ANIM_NODE_DATA(EAnimSyncMethod, Method); }
    bool GetIgnoreForRelevancyTest() const override { return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest); }
};

}
