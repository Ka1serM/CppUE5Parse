#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontageEvaluationState.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"
#include "ue5parse/runtime/Engine/Animation/AnimSync.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/BoneContainer.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;
class USkeleton;
struct FAnimNode_AssetPlayerRelevancyBase;
struct FAnimNode_LinkedInputPose;
struct FAnimNode_SaveCachedPose;
struct FAnimNode_StateMachine;

// UE's FAnimInstanceProxy (Animation/AnimInstanceProxy.h): the part of an anim instance that runs its graph. It links
// the node structs of the instance object, initializes them from the root, updates them each frame with the frame's
// tick records, and evaluates the pose. The game thread prepares each frame (PreUpdate); the rest can run on a worker.
class FAnimInstanceProxy {
public:
    FAnimInstanceProxy() = default;
    FAnimInstanceProxy(const FAnimInstanceProxy&) = delete;
    FAnimInstanceProxy& operator=(const FAnimInstanceProxy&) = delete;
    virtual ~FAnimInstanceProxy() = default;

    // Links the instance's nodes to their class data and finds the root, the cached poses and the default input pose.
    virtual void Initialize(UAnimInstance* InAnimInstance);
    // The component, the skeleton and, for a linked instance, the main instance's proxy.
    virtual void InitializeObjects(UAnimInstance* InAnimInstance);
    // Initializes the graph from the root, now or (deferred) at the start of the next update.
    void InitializeRootNode(bool bInDeferRootNodeInitialization = false);
    void RecalcRequiredBones(USkeletalMeshComponent* Component, UObject* Asset);
    // The passes of a graph from one of its roots: the main root, or the root of a graph another instance links to.
    void InitializeRootNode_WithRoot(FAnimNode_Base* InRootNode);
    void CacheBones_WithRoot(FAnimNode_Base* InRootNode);
    void UpdateAnimation_WithRoot(const FAnimationUpdateContext& InContext, FAnimNode_Base* InRootNode, FName InLayerName);
    void EvaluateAnimation_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode);
    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);
    // The graph update from the root, then the advance of the asset players.
    void UpdateAnimation();
    void EvaluateAnimation(FPoseContext& Output);
    // The pose of the graph (UE's Evaluate): a proxy without a graph overrides it.
    virtual bool Evaluate(FPoseContext& Output);
    // After the update: the weights it recorded become the ones the queries read, and the instance takes the notifies.
    virtual void PostUpdate(UAnimInstance* InAnimInstance);
    // Whether the asset players extract root motion, which locks the root of a root motion animation (UE's
    // ShouldExtractRootMotion, read from the instance's mode before each update).
    bool ShouldExtractRootMotion() const { return bShouldExtractRootMotion; }
    // The input pose of the main graph, which a post process instance takes from the main instance's output.
    FAnimNode_LinkedInputPose* GetDefaultLinkedInstanceInputNode() const { return DefaultLinkedInstanceInputNode; }
    // A state machine's state or transition notify, from the class's notify table.
    void AddAnimNotifyFromGeneratedClass(int32 NotifyIndex);
    FAnimNotifyQueue& GetNotifyQueue() { return NotifyQueue; }

    UObject* GetAnimInstanceObject() const { return AnimInstanceObject; }
    UAnimInstance* GetAnimInstance() const { return AnimInstanceObject; }
    const UAnimBlueprintGeneratedClass* GetAnimClassInterface() const { return AnimClassInterface; }
    USkeletalMeshComponent* GetSkelMeshComponent() const { return SkeletalMeshComponent; }
    USkeleton* GetSkeleton() const { return Skeleton; }
    const FBoneContainer& GetRequiredBones() const { return RequiredBones; }
    float GetDeltaSeconds() const { return CurrentDeltaSeconds; }
    // The component's world transform when the frame began, for the controls that work in world space.
    const FTransform& GetComponentTransform() const { return ComponentTransform; }
    // The owning actor's world transform when the frame began; the component's when it has no actor.
    const FTransform& GetActorTransform() const { return ActorTransform; }
    // The component's transform relative to its actor.
    const FTransform& GetComponentRelativeTransform() const { return ComponentRelativeTransform; }
    // A linked instance's players join the sync groups of the main instance, as in UE's shared sync scope.
    FAnimSync& GetSync() { return MainInstanceProxy != nullptr ? MainInstanceProxy->Sync : Sync; }
    const FGraphTraversalCounter& GetInitializationCounter() const { return InitializationCounter; }
    const FGraphTraversalCounter& GetCachedBonesCounter() const { return CachedBonesCounter; }
    const FGraphTraversalCounter& GetUpdateCounter() const { return UpdateCounter; }
    const FGraphTraversalCounter& GetEvaluationCounter() const { return EvaluationCounter; }

    // A node by its index in the class's state machines and player lists, which count from the end of the node
    // properties. Null for an index out of range or a node of another type.
    FAnimNode_Base* GetMutableNodeFromIndexUntyped(int32 NodeIdx, const UScriptStruct* RequiredStructType) const;
    template <class NodeType>
    NodeType* GetMutableNodeFromIndex(int32 NodeIdx) const {
        return static_cast<NodeType*>(GetMutableNodeFromIndexUntyped(NodeIdx, NodeType::StaticStruct()));
    }

    // The state machine weights of this update, by the machine's index in the class.
    void RecordMachineWeight(int32 InMachineClassIndex, float InMachineWeight);
    void RecordStateWeight(int32 InMachineClassIndex, int32 InStateIndex, float InStateWeight);
    // The script's queries. A machine index counts from the end of the node properties.
    const FAnimNode_StateMachine* GetStateMachineInstance(int32 MachineIndex) const;
    const FAnimNode_AssetPlayerRelevancyBase* GetRelevantAssetPlayerInterfaceFromState(int32 MachineIndex, int32 StateIndex) const;
    float GetRelevantAnimTimeRemaining(int32 MachineIndex, int32 StateIndex) const;
    float GetRelevantAnimTimeRemainingFraction(int32 MachineIndex, int32 StateIndex) const;
    float GetRelevantAnimLength(int32 MachineIndex, int32 StateIndex) const;
    float GetRelevantAnimTime(int32 MachineIndex, int32 StateIndex) const;
    float GetRelevantAnimTimeFraction(int32 MachineIndex, int32 StateIndex) const;
    float GetInstanceMachineWeight(int32 MachineIndex) const;
    float GetInstanceStateWeight(int32 MachineIndex, int32 StateIndex) const;
    float GetInstanceCurrentStateElapsedTime(int32 MachineIndex) const;

    // The montages of this frame, for the slot nodes.
    TArray<FMontageEvaluationState>& GetMontageEvaluationData() { return MontageEvaluationData; }
    const TArray<FMontageEvaluationState>& GetMontageEvaluationData() const { return MontageEvaluationData; }
    const FGraphTraversalCounter& GetSlotNodeInitializationCounter() const { return SlotNodeInitializationCounter; }
    // A slot node registers its name once per initialization of the instance; a second node of the name is ignored.
    void RegisterSlotNodeWithAnimInstance(const FName& SlotNodeName);
    void UpdateSlotNodeWeight(const FName& SlotNodeName, float InMontageLocalWeight, float InNodeGlobalWeight);
    bool IsSlotNodeRelevantForNotifies(const FName& SlotNodeName) const;
    // The slot weights of the last update.
    float GetSlotNodeGlobalWeight(const FName& SlotNodeName) const;
    float GetSlotMontageGlobalWeight(const FName& SlotNodeName) const;
    float GetSlotMontageLocalWeight(const FName& SlotNodeName) const;
    // The montages' weight in the slot (at most 1), the weight left for the source pose, and the sum of the montage
    // weights, which can be above 1.
    void GetSlotWeight(const FName& SlotNodeName, float& out_SlotNodeWeight, float& out_SourceWeight, float& out_TotalNodeWeight) const;
    // The source pose with the slot's montages: the non-additive ones blended with the source, then the additive ones.
    void SlotEvaluatePose(const FName& SlotNodeName, const FAnimationPoseData& SourceAnimationPoseData, float InSourceWeight,
                          FAnimationPoseData& OutBlendedAnimationPoseData, float InBlendWeight, float InTotalNodeWeight);

    void ReinitializeSlotNodes();

protected:
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext);

private:
    friend struct FAnimNode_LinkedAnimGraph;

    void CacheBones();
    // A linked instance's thread safe update, once per frame before its graph.
    void ThreadSafeUpdate();
    void ProcessCopies(int32 CallSite);
    void ClearSlotNodeWeights();

    UAnimInstance* AnimInstanceObject = nullptr;
    const UAnimBlueprintGeneratedClass* AnimClassInterface = nullptr;
    USkeleton* Skeleton = nullptr;
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
    FAnimNode_Base* RootNode = nullptr;
    FAnimNode_LinkedInputPose* DefaultLinkedInstanceInputNode = nullptr;
    // The main instance's proxy when this instance is a linked one.
    FAnimInstanceProxy* MainInstanceProxy = nullptr;
    // The nodes that read game thread data before the update (UE's GameThreadPreUpdateNodes).
    TArray<FAnimNode_Base*> GameThreadPreUpdateNodes;
    // The SaveCachedPose nodes of the main graph in update order.
    TArray<FAnimNode_SaveCachedPose*> SavedPoseQueue;
    FBoneContainer RequiredBones;
    FAnimSync Sync;
    FAnimNotifyQueue NotifyQueue;
    // Two buffers: the update writes one, the queries read the other (UE's buffer write index).
    TArray<float> MachineWeightArrays[2];
    TArray<float> StateWeightArrays[2];
    // The first state's entry in the state weights, by machine index in the class.
    TArray<int32> StateMachineWeightOffsets;
    int32 BufferWriteIndex = 0;
    TArray<FMontageEvaluationState> MontageEvaluationData;
    TMap<FName, int32> SlotNameToTrackerIndex;
    // Two buffers, as the state machine weights.
    TArray<FMontageActiveSlotTracker> SlotWeightTracker[2];
    FGraphTraversalCounter SlotNodeInitializationCounter;
    FGraphTraversalCounter InitializationCounter;
    FGraphTraversalCounter CachedBonesCounter;
    FGraphTraversalCounter UpdateCounter;
    FGraphTraversalCounter EvaluationCounter;
    FTransform ComponentTransform;
    FTransform ComponentRelativeTransform;
    FTransform ActorTransform;
    float CurrentDeltaSeconds = 0;
    bool bBoneCachesInvalidated = false;
    bool bDeferRootNodeInitialization = false;
    bool bThreadSafeUpdateDone = false;
    bool bShouldExtractRootMotion = false;
    int32 CacheBonesRecursionCounter = 0;
};

}
