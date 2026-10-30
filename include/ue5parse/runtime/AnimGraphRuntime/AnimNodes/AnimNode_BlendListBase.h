#pragma once

#include "ue5parse/runtime/Engine/AlphaBlend.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_BlendListBase.generated.h"

namespace ue5parse::runtime {

class UBlendProfile;

enum class EBlendListTransitionType : uint8 { StandardBlend, Inertialization };

// Blends to the active child over its blend time (UE's AnimNodes/AnimNode_BlendListBase.h). The subclasses say which
// child is active.
USTRUCT()
struct FAnimNode_BlendListBase : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FPoseLink> BlendPose;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    const TArray<float>& GetBlendTimes() const { return GET_ANIM_NODE_DATA(TArray<float>, BlendTime); }
    EBlendListTransitionType GetTransitionType() const { return GET_ANIM_NODE_DATA(EBlendListTransitionType, TransitionType); }
    EAlphaBlendOption GetBlendType() const { return GET_ANIM_NODE_DATA(EAlphaBlendOption, BlendType); }
    bool GetResetChildOnActivation() const { return GET_ANIM_NODE_DATA(bool, bResetChildOnActivation); }
    UCurveFloat* GetCustomBlendCurve() const { return GET_ANIM_NODE_DATA(UCurveFloat*, CustomBlendCurve); }
    UBlendProfile* GetBlendProfile() const { return GET_ANIM_NODE_DATA(UBlendProfile*, BlendProfile); }

protected:
    virtual int32 GetActiveChildIndex() { return 0; }

private:
    struct FBlendData {
        FAlphaBlend Blend;
        float Weight = 0;
        float RemainingTime = 0;
    };

    void Initialize();

    TArray<FBlendData> PerBlendData;
    int32 LastActiveChildIndex = INDEX_NONE;
};

}
