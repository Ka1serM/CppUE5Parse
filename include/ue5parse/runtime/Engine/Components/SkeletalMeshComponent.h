#pragma once

#include <memory>

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Components/SkinnedMeshComponent.h"
#include "ue5parse/runtime/Engine/ReferenceSkeleton.h"
#include "ue5parse/runtime/Engine/SingleAnimationPlayData.h"

#include "SkeletalMeshComponent.generated.h"

namespace ue5parse::runtime {

class UAnimInstance;
class UAnimSingleNodeInstance;
class UAnimationAsset;
class USkeletalMesh;
struct FCompactPose;
class FParallelAnimationEvaluationTask;

// UE's FAnimationEvaluationContext: what the evaluation task reads, and the buffers it writes. The component swaps its
// buffers with these around the task, so the game thread does not see a pose that is half written.
struct FAnimationEvaluationContext {
    const USkeletalMesh* SkeletalMesh = nullptr;
    UAnimInstance* AnimInstance = nullptr;
    UAnimInstance* PostProcessAnimInstance = nullptr;
    TArray<FTransform> ComponentSpaceTransforms;
    TArray<FTransform> BoneSpaceTransforms;
    FBlendedCurve Curve;
    bool bDoEvaluation = false;

    void Clear() {
        SkeletalMesh = nullptr;
        AnimInstance = nullptr;
        PostProcessAnimInstance = nullptr;
    }
};

namespace EAnimationMode {
enum Type : int32 { AnimationBlueprint, AnimationSingleNode, AnimationCustomMode };
}

// USkeletalMeshComponent: a placed skeletal mesh and the animation that poses it (UE's
// Components/SkeletalMeshComponent.h). The animation mode decides the main instance: one of the anim class, or a single
// node instance that plays AnimationData. The mesh's post process anim class runs after it on its output. Each tick
// updates the instances, and the evaluation fills the bone transforms.
UCLASS()
class USkeletalMeshComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EAnimationMode::Type> AnimationMode = EAnimationMode::AnimationBlueprint;
    UPROPERTY()
    TObjectPtr<UClass> AnimClass = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimInstance> AnimScriptInstance = nullptr;
    UPROPERTY(transient)
    TObjectPtr<UAnimInstance> PostProcessAnimInstance = nullptr;
    UPROPERTY()
    FSingleAnimationPlayData AnimationData;
    UPROPERTY()
    float GlobalAnimRateScale = 1.0f;
    UPROPERTY()
    bool bPauseAnims = false;
    UPROPERTY()
    bool bDisablePostProcessBlueprint = false;
    // The reference pose instead of the animation.
    UPROPERTY()
    bool bForceRefpose = false;
    UPROPERTY()
    bool bNoSkeletonUpdate = false;

    // The mesh bones the evaluation fills: every bone of the mesh, parents first.
    TArray<FBoneIndexType> RequiredBones;

    explicit USkeletalMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~USkeletalMeshComponent() override;
    void OnRegister() override;
    void OnUnregister() override;
    // TickPose, then RefreshBoneTransforms, which evaluates on a worker thread; the tick completes when the game thread
    // takes the result. A mesh that does not render skips them as its VisibilityBasedAnimTickOption says.
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable)
    USkeletalMesh* GetSkeletalMeshAsset() const;
    UFUNCTION(BlueprintCallable)
    void SetSkeletalMeshAsset(USkeletalMesh* NewMesh) { SetSkeletalMesh(NewMesh, false); }
    // Changes the mesh, then the animation starts again on it (InitAnim) when the component is registered.
    UFUNCTION(BlueprintCallable)
    void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true);
    void SetSkinnedAssetAndUpdate(USkinnedAsset* InSkinnedAsset, bool bReinitPose = true) override;
    UFUNCTION(BlueprintCallable)
    UAnimInstance* GetAnimInstance() const { return AnimScriptInstance; }
    UFUNCTION(BlueprintCallable)
    UAnimInstance* GetPostProcessInstance() const { return PostProcessAnimInstance; }
    UAnimSingleNodeInstance* GetSingleNodeInstance() const;
    // A new anim class makes the main instance again, in anim Blueprint mode (UE's SetAnimInstanceClass).
    UFUNCTION(BlueprintCallable)
    void SetAnimInstanceClass(UClass* NewClass);
    UFUNCTION(BlueprintCallable)
    void SetAnimClass(UClass* NewClass) { SetAnimInstanceClass(NewClass); }
    UFUNCTION(BlueprintCallable)
    UClass* GetAnimClass() { return AnimClass; }
    UFUNCTION(BlueprintCallable)
    void SetAnimationMode(EAnimationMode::Type InAnimationMode, bool bForceInitAnimScriptInstance = true);
    UFUNCTION(BlueprintCallable)
    EAnimationMode::Type GetAnimationMode() const { return AnimationMode; }
    // Single node mode: the asset, then playing it (UE's PlayAnimation).
    UFUNCTION(BlueprintCallable)
    void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);
    UFUNCTION(BlueprintCallable)
    void SetAnimation(UAnimationAsset* NewAnimToPlay);
    UFUNCTION(BlueprintCallable)
    void Play(bool bLooping);
    UFUNCTION(BlueprintCallable)
    void Stop();
    UFUNCTION(BlueprintCallable)
    bool IsPlaying() const;
    UFUNCTION(BlueprintCallable)
    void SetPosition(float InPos, bool bFireNotifies = true);
    UFUNCTION(BlueprintCallable)
    float GetPosition() const;
    UFUNCTION(BlueprintCallable)
    void SetPlayRate(float Rate);
    UFUNCTION(BlueprintCallable)
    float GetPlayRate() const;
    UFUNCTION(BlueprintCallable)
    void OverrideAnimationData(UAnimationAsset* InAnimToPlay, bool bIsLooping = true, bool bIsPlaying = true, float Position = 0.f, float PlayRate = 1.f);
    UFUNCTION(BlueprintCallable)
    void SetDisablePostProcessBlueprint(bool bInDisablePostProcess);
    UFUNCTION(BlueprintCallable)
    bool GetDisablePostProcessBlueprint() const { return bDisablePostProcessBlueprint; }
    // The LOD the notify filters compare against: the most detailed one, as nothing renders.
    int32 GetPredictedLODLevel() const { return 0; }
    // The main instance, the linked ones and the post process instance (UE's ForEachAnimInstance).
    template <class Function>
    void ForEachAnimInstance(Function&& InFunction) {
        if (AnimScriptInstance != nullptr) InFunction(AnimScriptInstance);
        for (UAnimInstance* LinkedInstance : LinkedInstances) InFunction(LinkedInstance);
        if (PostProcessAnimInstance != nullptr) InFunction(PostProcessAnimInstance);
    }
    // The instances that the main instance's linked graph nodes made.
    TArray<UAnimInstance*>& GetLinkedAnimInstances() { return LinkedInstances; }
    const TArray<UAnimInstance*>& GetLinkedAnimInstances() const { return LinkedInstances; }

    // Makes the instances that the animation mode and the mesh ask for, unless they exist (bForceReinit
    // initializes them again), then sets the reference pose. A component that is not registered has none.
    void InitAnim(bool bForceReinit);
    // The main and post process instances: true when one was initialized (UE's InitializeAnimScriptInstance).
    bool InitializeAnimScriptInstance(bool bForceReinit, bool bInDeferRootNodeInitialization = false);
    // Ends the main instance's notify states and drops it and the linked instances.
    void ClearAnimScriptInstance();
    // The game thread's part of the animation tick: the linked, main and post process instances' events.
    void TickAnimation(float DeltaTime);
    // The graph updates and evaluations (the main instance's, then the post process instance's on its output), then
    // the component space transforms, the post evaluation events and the queued anim events. With a tick function, the
    // updates and evaluations run on a worker thread, and the rest when the tick completes.
    void RefreshBoneTransforms(FActorComponentTickFunction* TickFunction = nullptr);
    bool IsRunningParallelEvaluation() const { return bRunningParallelEvaluation; }
    // Waits for a running evaluation task when bBlockOnTask, then takes its result: true when a task was running.
    bool HandleExistingParallelEvaluationTask(bool bBlockOnTask, bool bPerformPostAnimEvaluation);
    // The queued notifies and montage events of the frame, once per update (UE's ConditionallyDispatchQueuedAnimEvents).
    void ConditionallyDispatchQueuedAnimEvents();
    // The curves of the last evaluation.
    const FBlendedCurve& GetAnimationCurves() const { return AnimCurves; }

    // The evaluated local transform of each mesh bone, after a running evaluation completes.
    const TArray<FTransform>& GetBoneSpaceTransforms();
    // The transform of each mesh bone in component space, as the last completed evaluation left it.
    const TArray<FTransform>& GetComponentSpaceTransforms() const { return ComponentSpaceTransformsArray[CurrentReadComponentTransforms]; }

private:
    void RecalcRequiredBones();
    bool NeedToSpawnAnimScriptInstance() const;
    bool NeedToSpawnPostPhysicsInstance(bool bForceReinit) const;
    // Destroys the linked instances that the linked graph nodes made.
    void ResetLinkedAnimInstances();
    bool ShouldUpdatePostProcessInstance() const;
    bool ShouldEvaluatePostProcessInstance() const;
    // The main pose into the post process instance's input node, then its evaluation (UE's
    // EvaluatePostProcessMeshInstance).
    void EvaluatePostProcessMeshInstance(FCompactPose& InOutPose, FBlendedCurve& InOutCurve) const;
    // The updates and the evaluation into the buffers, on any thread (UE's PerformAnimationProcessing).
    void PerformAnimationProcessing(const USkeletalMesh* InSkeletalMesh, UAnimInstance* InAnimInstance, bool bInDoEvaluation,
                                    TArray<FTransform>& OutSpaceBases, TArray<FTransform>& OutBoneSpaceTransforms, FBlendedCurve& OutCurve) const;
    void ParallelAnimationEvaluation();
    void SwapEvaluationContextBuffers();
    void DispatchParallelEvaluationTasks(FActorComponentTickFunction* TickFunction);
    void DoParallelEvaluationTasks_OnGameThread();
    void CompleteParallelAnimationEvaluation(bool bDoPostAnimEvaluation);
    // On the game thread after the evaluation: the post updates, the curves, the post evaluation events, then
    // FinalizeAnimationUpdate.
    void PostAnimEvaluation(FAnimationEvaluationContext& EvaluationContext);
    void DoInstancePreEvaluation();
    void DoInstancePostEvaluation();
    // The new bones become the ones to read, then the queued events and the attached children follow them.
    void FinalizeAnimationUpdate();
    void FinalizeBoneTransform();
    void FlipEditableSpaceBases();
    TArray<FTransform>& GetEditableComponentSpaceTransforms() { return ComponentSpaceTransformsArray[CurrentEditableComponentTransforms]; }

    TArray<UAnimInstance*> LinkedInstances;
    FBlendedCurve AnimCurves;
    bool bNeedsQueuedAnimEventsDispatched = false;
    TArray<FTransform> BoneSpaceTransforms;
    // Double buffered, so that a reader during the evaluation sees the last complete pose.
    TArray<FTransform> ComponentSpaceTransformsArray[2];
    int32 CurrentEditableComponentTransforms = 0;
    int32 CurrentReadComponentTransforms = 1;
    bool bNeedToFlipSpaceBaseBuffers = false;
    bool bHasValidBoneTransform = false;
    FAnimationEvaluationContext AnimEvaluationContext;
    std::unique_ptr<FParallelAnimationEvaluationTask> ParallelAnimationEvaluationTask;
    bool bRunningParallelEvaluation = false;
};

}
