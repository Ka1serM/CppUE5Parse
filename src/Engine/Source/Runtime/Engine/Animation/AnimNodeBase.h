#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeData.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationPoseData.h"
#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

#include "AnimNodeBase.generated.h"

namespace ue5parse::runtime {

class FAnimInstanceProxy;
class UAnimBlueprintGeneratedClass;
struct FAnimNode_Base;

struct FAnimationBaseContext {
    FAnimInstanceProxy* AnimInstanceProxy = nullptr;

    explicit FAnimationBaseContext(FAnimInstanceProxy* InAnimInstanceProxy) : AnimInstanceProxy(InAnimInstanceProxy) {}
    const UAnimBlueprintGeneratedClass* GetAnimClass() const;
    UObject* GetAnimInstanceObject() const;
};

struct FAnimationInitializeContext : FAnimationBaseContext {
    using FAnimationBaseContext::FAnimationBaseContext;
};

struct FAnimationCacheBonesContext : FAnimationBaseContext {
    using FAnimationBaseContext::FAnimationBaseContext;
};

struct FAnimationUpdateContext : FAnimationBaseContext {
    FAnimationUpdateContext(FAnimInstanceProxy* InAnimInstanceProxy, float InDeltaTime) : FAnimationBaseContext(InAnimInstanceProxy), DeltaTime(InDeltaTime) {}

    FAnimationUpdateContext FractionalWeight(float WeightMultiplier) const {
        FAnimationUpdateContext Result(*this);
        Result.CurrentWeight = CurrentWeight * WeightMultiplier;
        return Result;
    }
    FAnimationUpdateContext FractionalWeightAndRootMotion(float WeightMultiplier, float RootMotionMultiplier) const {
        FAnimationUpdateContext Result(FractionalWeight(WeightMultiplier));
        Result.RootMotionWeightModifier = RootMotionWeightModifier * RootMotionMultiplier;
        return Result;
    }
    FAnimationUpdateContext WithOtherProxy(FAnimInstanceProxy* InAnimInstanceProxy) const {
        FAnimationUpdateContext Result(*this);
        Result.AnimInstanceProxy = InAnimInstanceProxy;
        return Result;
    }
    bool IsActive() const { return bIsActive; }
    float GetDeltaTime() const { return DeltaTime; }
    float GetFinalBlendWeight() const { return CurrentWeight; }
    float GetRootMotionWeightModifier() const { return RootMotionWeightModifier; }

private:
    float CurrentWeight = 1.0f;
    float RootMotionWeightModifier = 1.0f;
    float DeltaTime = 0;
    bool bIsActive = true;
};

struct FPoseContext : FAnimationBaseContext {
    FCompactPose Pose;
    FBlendedCurve Curve;

    explicit FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose = false);
    explicit FPoseContext(const FPoseContext& SourceContext, bool bOverrideAdditive = false);
    FPoseContext& operator=(const FPoseContext& Other) {
        Pose.CopyBonesFrom(Other.Pose);
        Curve.CopyFrom(Other.Curve);
        return *this;
    }

    void ResetToRefPose() {
        if (bExpectsAdditivePose)
            Pose.ResetToAdditiveIdentity();
        else
            Pose.ResetToRefPose();
    }
    void ResetToAdditiveIdentity() { Pose.ResetToAdditiveIdentity(); }
    bool ExpectsAdditivePose() const { return bExpectsAdditivePose; }
    FAnimationPoseData GetAnimationPoseData() { return FAnimationPoseData(Pose, Curve); }

private:
    friend struct FScopedExpectsAdditiveOverride;
    bool bExpectsAdditivePose = false;
};

struct FScopedExpectsAdditiveOverride {
    FScopedExpectsAdditiveOverride(FPoseContext& InContext, bool bInExpectsAdditive) : Context(InContext), bPreviousValue(InContext.bExpectsAdditivePose) {
        Context.bExpectsAdditivePose = bInExpectsAdditive;
    }
    ~FScopedExpectsAdditiveOverride() { Context.bExpectsAdditivePose = bPreviousValue; }
    FScopedExpectsAdditiveOverride(const FScopedExpectsAdditiveOverride&) = delete;
    FScopedExpectsAdditiveOverride& operator=(const FScopedExpectsAdditiveOverride&) = delete;

private:
    FPoseContext& Context;
    bool bPreviousValue;
};

struct FComponentSpacePoseContext : FAnimationBaseContext {
    FCSPose<FCompactPose> Pose;
    FBlendedCurve Curve;

    explicit FComponentSpacePoseContext(FAnimInstanceProxy* InAnimInstanceProxy);
    void ResetToRefPose();
};

USTRUCT()
struct FPoseLinkBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 LinkID = INDEX_NONE;

    void Initialize(const FAnimationInitializeContext& Context);
    void CacheBones(const FAnimationCacheBonesContext& Context);
    void Update(const FAnimationUpdateContext& Context);
    void AttemptRelink(const FAnimationBaseContext& Context);
    void SetDynamicLinkNode(FPoseLinkBase* InPoseLink);

protected:
    FAnimNode_Base* LinkedNode = nullptr;
};

USTRUCT()
struct FPoseLink : public FPoseLinkBase {
    GENERATED_BODY()
public:
    void Evaluate(FPoseContext& Output);
};

USTRUCT()
struct FComponentSpacePoseLink : public FPoseLinkBase {
    GENERATED_BODY()
public:
    void EvaluateComponentSpace(FComponentSpacePoseContext& Output);
};

namespace UE::Anim {

struct FNodeDataId {
    FNodeDataId() = default;
    FNodeDataId(FName InPropertyName, const FAnimNode_Base* InNode, const UScriptStruct* InNodeStruct);
    bool IsValid() const { return Index != INDEX_NONE; }
    int32 GetIndex() const { return Index; }

private:
    int32 Index = INDEX_NONE;
};

}

USTRUCT()
struct FAnimNode_Base {
    GENERATED_BODY()
public:
    FAnimNode_Base() = default;
    FAnimNode_Base(const FAnimNode_Base&) = default;
    FAnimNode_Base& operator=(const FAnimNode_Base&) = default;
    virtual ~FAnimNode_Base() = default;

    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) {}
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {}
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) {}
    virtual void Evaluate_AnyThread(FPoseContext& Output);
    virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output);
    virtual bool HasPreUpdate() const { return false; }
    virtual void PreUpdate(const class UAnimInstance* InAnimInstance) {}
    virtual bool NeedsOnInitializeAnimInstance() const { return false; }
    virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const class UAnimInstance* InAnimInstance) {}

    void SetNodeData(const FAnimNodeData& InNodeData) { NodeData = &InNodeData; }
    const FAnimNodeData* GetNodeData() const { return NodeData; }
    const struct FExposedValueHandler& GetEvaluateGraphExposedInputs() const;

protected:
    template <typename DataType>
    const DataType& GetData(UE::Anim::FNodeDataId InId) const {
        return *static_cast<const DataType*>(NodeData->GetData(InId, this));
    }
    template <typename DataType>
    DataType* GetInstanceDataPtr(UE::Anim::FNodeDataId InId) {
        return static_cast<DataType*>(const_cast<void*>(NodeData->GetInstanceData(InId, this)));
    }

    const FAnimNodeData* NodeData = nullptr;
};

}

#define GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)                                                                                                       \
    [this]() -> ::ue5parse::runtime::UE::Anim::FNodeDataId {                                                                                                   \
        static ::ue5parse::runtime::UE::Anim::FNodeDataId CachedId_##Identifier;                                                                               \
        if (!CachedId_##Identifier.IsValid())                                                                                                                  \
            CachedId_##Identifier = ::ue5parse::runtime::UE::Anim::FNodeDataId(::ue5parse::runtime::FName(#Identifier), this, StaticStruct());                 \
        return CachedId_##Identifier;                                                                                                                          \
    }()

#define GET_ANIM_NODE_DATA(Type, Identifier) (GetData<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))
#define GET_INSTANCE_ANIM_NODE_DATA_PTR(Type, Identifier) (GetInstanceDataPtr<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))
