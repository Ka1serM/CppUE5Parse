#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimMontageEvaluationState.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifyQueue.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSync.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/BoneContainer.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;
class USkeleton;
struct FAnimNode_AssetPlayerRelevancyBase;
struct FAnimNode_LinkedInputPose;
struct FAnimNode_SaveCachedPose;
struct FAnimNode_StateMachine;

class FAnimInstanceProxy {
public:
    FAnimInstanceProxy() = default;
    FAnimInstanceProxy(const FAnimInstanceProxy&) = delete;
    FAnimInstanceProxy& operator=(const FAnimInstanceProxy&) = delete;
    virtual ~FAnimInstanceProxy() = default;

    virtual void Initialize(UAnimInstance* InAnimInstance);
    virtual void InitializeObjects(UAnimInstance* InAnimInstance);
    void InitializeRootNode(bool bInDeferRootNodeInitialization = false);
    void RecalcRequiredBones(USkeletalMeshComponent* Component, UObject* Asset);
    void InitializeRootNode_WithRoot(FAnimNode_Base* InRootNode);
    void CacheBones_WithRoot(FAnimNode_Base* InRootNode);
    void UpdateAnimation_WithRoot(const FAnimationUpdateContext& InContext, FAnimNode_Base* InRootNode, FName InLayerName);
    void EvaluateAnimation_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode);
    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);
    void UpdateAnimation();
    void EvaluateAnimation(FPoseContext& Output);
    virtual bool Evaluate(FPoseContext& Output);
    virtual void PostUpdate(UAnimInstance* InAnimInstance);
    bool ShouldExtractRootMotion() const { return bShouldExtractRootMotion; }
    FAnimNode_LinkedInputPose* GetDefaultLinkedInstanceInputNode() const { return DefaultLinkedInstanceInputNode; }
    void AddAnimNotifyFromGeneratedClass(int32 NotifyIndex);

    UObject* GetAnimInstanceObject() const { return AnimInstanceObject; }
    UAnimInstance* GetAnimInstance() const { return AnimInstanceObject; }
    const UAnimBlueprintGeneratedClass* GetAnimClassInterface() const { return AnimClassInterface; }
    USkeletalMeshComponent* GetSkelMeshComponent() const { return SkeletalMeshComponent; }
    USkeleton* GetSkeleton() const { return Skeleton; }
    const FBoneContainer& GetRequiredBones() const { return RequiredBones; }
    float GetDeltaSeconds() const { return CurrentDeltaSeconds; }
    const FTransform& GetComponentTransform() const { return ComponentTransform; }
    const FTransform& GetActorTransform() const { return ActorTransform; }
    FAnimSync& GetSync() { return MainInstanceProxy != nullptr ? MainInstanceProxy->Sync : Sync; }
    const FGraphTraversalCounter& GetInitializationCounter() const { return InitializationCounter; }
    const FGraphTraversalCounter& GetCachedBonesCounter() const { return CachedBonesCounter; }
    const FGraphTraversalCounter& GetUpdateCounter() const { return UpdateCounter; }
    const FGraphTraversalCounter& GetEvaluationCounter() const { return EvaluationCounter; }

    FAnimNode_Base* GetMutableNodeFromIndexUntyped(int32 NodeIdx, const UScriptStruct* RequiredStructType) const;
    template <class NodeType>
    NodeType* GetMutableNodeFromIndex(int32 NodeIdx) const {
        return static_cast<NodeType*>(GetMutableNodeFromIndexUntyped(NodeIdx, NodeType::StaticStruct()));
    }

    void RecordMachineWeight(int32 InMachineClassIndex, float InMachineWeight);
    void RecordStateWeight(int32 InMachineClassIndex, int32 InStateIndex, float InStateWeight);
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

    TArray<FMontageEvaluationState>& GetMontageEvaluationData() { return MontageEvaluationData; }
    const TArray<FMontageEvaluationState>& GetMontageEvaluationData() const { return MontageEvaluationData; }
    const FGraphTraversalCounter& GetSlotNodeInitializationCounter() const { return SlotNodeInitializationCounter; }
    void RegisterSlotNodeWithAnimInstance(const FName& SlotNodeName);
    void UpdateSlotNodeWeight(const FName& SlotNodeName, float InMontageLocalWeight, float InNodeGlobalWeight);
    bool IsSlotNodeRelevantForNotifies(const FName& SlotNodeName) const;
    float GetSlotNodeGlobalWeight(const FName& SlotNodeName) const;
    float GetSlotMontageGlobalWeight(const FName& SlotNodeName) const;
    float GetSlotMontageLocalWeight(const FName& SlotNodeName) const;
    void GetSlotWeight(const FName& SlotNodeName, float& out_SlotNodeWeight, float& out_SourceWeight, float& out_TotalNodeWeight) const;
    void SlotEvaluatePose(const FName& SlotNodeName, const FAnimationPoseData& SourceAnimationPoseData, float InSourceWeight,
                          FAnimationPoseData& OutBlendedAnimationPoseData, float InBlendWeight, float InTotalNodeWeight);

    void ReinitializeSlotNodes();

protected:
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext);

private:
    friend struct FAnimNode_LinkedAnimGraph;

    void CacheBones();
    void ThreadSafeUpdate();
    void ProcessCopies(int32 CallSite);
    void ClearSlotNodeWeights();

    UAnimInstance* AnimInstanceObject = nullptr;
    const UAnimBlueprintGeneratedClass* AnimClassInterface = nullptr;
    USkeleton* Skeleton = nullptr;
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
    FAnimNode_Base* RootNode = nullptr;
    FAnimNode_LinkedInputPose* DefaultLinkedInstanceInputNode = nullptr;
    FAnimInstanceProxy* MainInstanceProxy = nullptr;
    TArray<FAnimNode_Base*> GameThreadPreUpdateNodes;
    TArray<FAnimNode_SaveCachedPose*> SavedPoseQueue;
    FBoneContainer RequiredBones;
    FAnimSync Sync;
    FAnimNotifyQueue NotifyQueue;
    TArray<float> MachineWeightArrays[2];
    TArray<float> StateWeightArrays[2];
    TArray<int32> StateMachineWeightOffsets;
    int32 BufferWriteIndex = 0;
    TArray<FMontageEvaluationState> MontageEvaluationData;
    TMap<FName, int32> SlotNameToTrackerIndex;
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
