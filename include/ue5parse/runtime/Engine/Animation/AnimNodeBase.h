#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeData.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"
#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"
#include "ue5parse/runtime/Engine/BonePose.h"

#include "AnimNodeBase.generated.h"

// The anim graph's node interface (UE's Animation/AnimNodeBase.h): the contexts of the four passes, the links between
// nodes, and FAnimNode_Base. A cooked anim Blueprint holds its nodes as struct properties of the instance object; the
// links resolve to those structs.
namespace ue5parse::runtime {

class FAnimInstanceProxy;
class UAnimBlueprintGeneratedClass;
struct FAnimNode_Base;

// The context every pass shares: the proxy of the instance being run.
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

// The update pass's context: the frame's delta time and the weight of the branch being updated.
struct FAnimationUpdateContext : FAnimationBaseContext {
    FAnimationUpdateContext(FAnimInstanceProxy* InAnimInstanceProxy, float InDeltaTime)
        : FAnimationBaseContext(InAnimInstanceProxy), DeltaTime(InDeltaTime) {}

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
    FAnimationUpdateContext FractionalWeightAndTime(float WeightMultiplier, float TimeMultiplier) const {
        FAnimationUpdateContext Result(FractionalWeight(WeightMultiplier));
        Result.DeltaTime = DeltaTime * TimeMultiplier;
        return Result;
    }
    // The same update for another instance's graph (a linked graph and its input poses).
    FAnimationUpdateContext WithOtherProxy(FAnimInstanceProxy* InAnimInstanceProxy) const {
        FAnimationUpdateContext Result(*this);
        Result.AnimInstanceProxy = InAnimInstanceProxy;
        return Result;
    }
    // A branch that blends out: a state that a transition leaves, the old child of a blend list.
    FAnimationUpdateContext AsInactive() const {
        FAnimationUpdateContext Result(*this);
        Result.bIsActive = false;
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

// The evaluation pass's context in local space: the pose and the curves that the node writes.
struct FPoseContext : FAnimationBaseContext {
    FCompactPose Pose;
    FBlendedCurve Curve;

    // A pose for the proxy's required bones. An additive pose resets to the additive identity, not the reference pose.
    explicit FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose = false);
    // A pose of the same proxy, which also expects an additive pose when SourceContext does or bOverrideAdditive is set.
    explicit FPoseContext(const FPoseContext& SourceContext, bool bOverrideAdditive = false);
    FPoseContext& operator=(const FPoseContext& Other) {
        Pose.CopyBonesFrom(Other.Pose);
        Curve.CopyFrom(Other.Curve);
        return *this;
    }

    void ResetToRefPose() {
        if (bExpectsAdditivePose) Pose.ResetToAdditiveIdentity();
        else Pose.ResetToRefPose();
    }
    void ResetToAdditiveIdentity() { Pose.ResetToAdditiveIdentity(); }
    bool ExpectsAdditivePose() const { return bExpectsAdditivePose; }
    FAnimationPoseData GetAnimationPoseData() { return FAnimationPoseData(Pose, Curve); }

private:
    friend struct FScopedExpectsAdditiveOverride;
    bool bExpectsAdditivePose = false;
};

// Sets whether a pose context expects an additive pose for a scope, then restores it.
struct FScopedExpectsAdditiveOverride {
    FScopedExpectsAdditiveOverride(FPoseContext& InContext, bool bInExpectsAdditive)
        : Context(InContext), bPreviousValue(InContext.bExpectsAdditivePose) {
        Context.bExpectsAdditivePose = bInExpectsAdditive;
    }
    ~FScopedExpectsAdditiveOverride() { Context.bExpectsAdditivePose = bPreviousValue; }
    FScopedExpectsAdditiveOverride(const FScopedExpectsAdditiveOverride&) = delete;
    FScopedExpectsAdditiveOverride& operator=(const FScopedExpectsAdditiveOverride&) = delete;

private:
    FPoseContext& Context;
    bool bPreviousValue;
};

// The evaluation pass's context in component space.
struct FComponentSpacePoseContext : FAnimationBaseContext {
    FCSPose<FCompactPose> Pose;
    FBlendedCurve Curve;

    explicit FComponentSpacePoseContext(FAnimInstanceProxy* InAnimInstanceProxy);
    void ResetToRefPose();
};

// A link from a node's input to another node of the same instance: LinkID is the index of the linked node in the
// class's node properties. It resolves to the linked struct on first use.
USTRUCT()
struct FPoseLinkBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 LinkID = INDEX_NONE;

    void Initialize(const FAnimationInitializeContext& Context);
    void CacheBones(const FAnimationCacheBonesContext& Context);
    void Update(const FAnimationUpdateContext& Context);
    // Resolves LinkID to the node of the context's instance.
    void AttemptRelink(const FAnimationBaseContext& Context);
    FAnimNode_Base* GetLinkNode() { return LinkedNode; }
    void SetLinkNode(FAnimNode_Base* NewLinkNode) { LinkedNode = NewLinkNode; }
    // Takes the target of another link (a linked graph's input), or none.
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

// The index of a folded node property in its node type's layout (UE's FNodeDataId). Every class has the same layout
// for a node type, so a call site resolves its id once (GET_ANIM_NODE_DATA).
struct FNodeDataId {
    FNodeDataId() = default;
    FNodeDataId(FName InPropertyName, const FAnimNode_Base* InNode, const UScriptStruct* InNodeStruct);
    bool IsValid() const { return Index != INDEX_NONE; }
    int32 GetIndex() const { return Index; }

private:
    int32 Index = INDEX_NONE;
};

}

// The base of every anim graph node. The four passes run from the root: Initialize when the graph or a branch starts,
// CacheBones when the required bones change, Update once per frame with the branch's weight, and Evaluate for the
// pose. A node reads a folded property (a constant of the class, or a value the instance changes) through
// GET_ANIM_NODE_DATA.
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
    // Called on the instance's thread before the graph updates, for the nodes whose HasPreUpdate is true.
    virtual bool HasPreUpdate() const { return false; }
    virtual void PreUpdate(const class UAnimInstance* InAnimInstance) {}
    // Called on the instance's thread when the instance initializes, for the nodes whose
    // NeedsOnInitializeAnimInstance is true (to read the mesh or the instance's objects).
    virtual bool NeedsOnInitializeAnimInstance() const { return false; }
    virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const class UAnimInstance* InAnimInstance) {}

    // The node's entry of the class's node data; set when the instance links its nodes.
    void SetNodeData(const FAnimNodeData& InNodeData) { NodeData = &InNodeData; }
    const FAnimNodeData* GetNodeData() const { return NodeData; }
    // The handler that copies the node's bound inputs from the instance. A node runs it in its Initialize and Update.
    const struct FExposedValueHandler& GetEvaluateGraphExposedInputs() const;

protected:
    template <typename DataType>
    const DataType& GetData(UE::Anim::FNodeDataId InId) const {
        return *static_cast<const DataType*>(NodeData->GetData(InId, this));
    }
    // The folded value when the instance holds it, else null (a constant of the class).
    template <typename DataType>
    DataType* GetInstanceDataPtr(UE::Anim::FNodeDataId InId) {
        return static_cast<DataType*>(const_cast<void*>(NodeData->GetInstanceData(InId, this)));
    }

    const FAnimNodeData* NodeData = nullptr;
};

}

// The id of a folded property of the calling node's type, resolved once per call site (UE's
// GET_ANIM_NODE_DATA_ID_INTERNAL).
#define GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier) \
    [this]() -> ::ue5parse::runtime::UE::Anim::FNodeDataId { \
        static ::ue5parse::runtime::UE::Anim::FNodeDataId CachedId_##Identifier; \
        if (!CachedId_##Identifier.IsValid()) \
            CachedId_##Identifier = ::ue5parse::runtime::UE::Anim::FNodeDataId(::ue5parse::runtime::FName(#Identifier), this, StaticStruct()); \
        return CachedId_##Identifier; \
    }()

// A folded property of the calling node: the class's constant or the instance's value.
#define GET_ANIM_NODE_DATA(Type, Identifier) (GetData<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))
#define GET_INSTANCE_ANIM_NODE_DATA_PTR(Type, Identifier) (GetInstanceDataPtr<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))
