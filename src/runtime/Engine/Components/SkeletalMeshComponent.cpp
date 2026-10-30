#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

#include <utility>

#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/task_group.h>

#include "ue5parse/core/Check.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedInputPose.h"
#include "ue5parse/runtime/Engine/Animation/AnimSingleNodeInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

// UE's Components/SkeletalMeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

// UE's FParallelAnimationEvaluationTask: the worker task of one component, which the game thread waits on.
class FParallelAnimationEvaluationTask {
public:
    tbb::task_group Group;
    // The game thread and two workers. The tasks are small and the game thread makes them one by one, so more workers
    // only spin: on the round 3 replay, 32 threads took 40 s and 576 s of system time, 3 threads took 29 s.
    static tbb::task_arena& Arena() {
        static tbb::task_arena Instance(3);
        return Instance;
    }
};

namespace {

// Marks the thread while it runs an evaluation task.
class FParallelAnimationEvaluationScope {
public:
    FParallelAnimationEvaluationScope() { bActive = true; }
    ~FParallelAnimationEvaluationScope() { bActive = false; }
    FParallelAnimationEvaluationScope(const FParallelAnimationEvaluationScope&) = delete;
    FParallelAnimationEvaluationScope& operator=(const FParallelAnimationEvaluationScope&) = delete;
    static bool IsActive() { return bActive; }

private:
    static inline thread_local bool bActive = false;
};

}

USkeletalMeshComponent::USkeletalMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

USkeletalMeshComponent::~USkeletalMeshComponent() { HandleExistingParallelEvaluationTask(true, false); }

void USkeletalMeshComponent::OnRegister() {
    Super::OnRegister();
    // Registration starts from no linked instances and initializes the instances again.
    ResetLinkedAnimInstances();
    InitAnim(true);
}

void USkeletalMeshComponent::OnUnregister() {
    // The work of a running task is of no use to a component that goes away.
    HandleExistingParallelEvaluationTask(true, false);
    Super::OnUnregister();
}

void USkeletalMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (ShouldTickPose()) TickAnimation(DeltaTime);
    if (ShouldUpdateTransform()) RefreshBoneTransforms(ThisTickFunction);
    // UE's tick.AnimationDelaysEndGroup: with no physics to blend, the evaluation can complete as late as
    // TG_PostPhysics.
    if (ThisTickFunction != nullptr) ThisTickFunction->EndTickGroup = TG_PostPhysics;
    // A completed evaluation dispatched the events after its pose; a tick that did not evaluate dispatches them here.
    if (!IsRunningParallelEvaluation()) ConditionallyDispatchQueuedAnimEvents();
}

const TArray<FTransform>& USkeletalMeshComponent::GetBoneSpaceTransforms() {
    HandleExistingParallelEvaluationTask(true, true);
    return BoneSpaceTransforms;
}

USkeletalMesh* USkeletalMeshComponent::GetSkeletalMeshAsset() const { return Cast<USkeletalMesh>(GetSkinnedAsset()); }

void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose) {
    if (NewMesh == GetSkeletalMeshAsset()) return;
    HandleExistingParallelEvaluationTask(true, false);
    USkinnedMeshComponent::SetSkinnedAssetAndUpdate(NewMesh, bReinitPose);
    // UE's InitAnim does nothing on a component that is not registered.
    if (IsRegistered()) InitAnim(bReinitPose);
}

void USkeletalMeshComponent::SetSkinnedAssetAndUpdate(USkinnedAsset* InSkinnedAsset, bool bReinitPose) {
    SetSkeletalMesh(Cast<USkeletalMesh>(InSkinnedAsset), bReinitPose);
}

void USkeletalMeshComponent::RecalcRequiredBones() {
    RequiredBones.Empty();
    const USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (Mesh == nullptr) return;
    const int32 NumBones = Mesh->GetRefSkeleton().GetNum();
    RequiredBones.Reserve(NumBones);
    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex) RequiredBones.Add(static_cast<FBoneIndexType>(BoneIndex));
}

bool USkeletalMeshComponent::NeedToSpawnAnimScriptInstance() const {
    const USkeletalMesh* MeshAsset = GetSkeletalMeshAsset();
    const auto* AnimClassInterface = Cast<UAnimBlueprintGeneratedClass>(AnimClass);
    const USkeleton* AnimSkeleton = AnimClassInterface ? AnimClassInterface->GetTargetSkeleton() : nullptr;
    // A template anim Blueprint has no skeleton: the mesh's.
    if (AnimSkeleton == nullptr) AnimSkeleton = MeshAsset ? MeshAsset->GetSkeleton() : nullptr;
    const bool bSkeletonsExist = MeshAsset && AnimSkeleton && MeshAsset->GetSkeleton();
    const bool bAnimSkelValid = AnimClassInterface == nullptr || bSkeletonsExist;
    if (AnimationMode != EAnimationMode::AnimationBlueprint || AnimClass == nullptr || !bAnimSkelValid) return false;
    return AnimScriptInstance == nullptr || AnimScriptInstance->GetClass() != AnimClass || AnimScriptInstance->GetOuter() != this;
}

bool USkeletalMeshComponent::NeedToSpawnPostPhysicsInstance(bool bForceReinit) const {
    const USkeletalMesh* MeshAsset = GetSkeletalMeshAsset();
    if (MeshAsset == nullptr) return false;
    const UClass* ClassToUse = MeshAsset->GetPostProcessAnimBlueprint();
    const UClass* CurrentClass = PostProcessAnimInstance ? PostProcessAnimInstance->GetClass() : nullptr;
    const auto* AnimClassInterface = Cast<UAnimBlueprintGeneratedClass>(ClassToUse);
    const USkeleton* AnimSkeleton = AnimClassInterface ? AnimClassInterface->GetTargetSkeleton() : nullptr;
    if (AnimSkeleton == nullptr) AnimSkeleton = MeshAsset->GetSkeleton();
    return ClassToUse != nullptr && (ClassToUse != CurrentClass || bForceReinit) && AnimClass != ClassToUse && AnimSkeleton != nullptr;
}

void USkeletalMeshComponent::InitAnim(bool bForceReinit) {
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (Mesh == nullptr || !IsRegistered()) return;
    HandleExistingParallelEvaluationTask(true, false);
    const bool bBlueprintMismatch = AnimClass != nullptr && AnimScriptInstance != nullptr && AnimScriptInstance->GetClass() != AnimClass;
    const USkeleton* AnimSkeleton = AnimScriptInstance ? AnimScriptInstance->CurrentSkeleton : nullptr;
    const bool bClearAnimInstance = AnimScriptInstance && AnimSkeleton == nullptr;
    const bool bSkeletonMismatch = AnimSkeleton && AnimScriptInstance->CurrentSkeleton != Mesh->GetSkeleton();
    const bool bSkeletonsExist = AnimSkeleton && Mesh->GetSkeleton() && !bSkeletonMismatch;
    if (bBlueprintMismatch || bSkeletonMismatch || !bSkeletonsExist || bClearAnimInstance) ClearAnimScriptInstance();
    RecalcRequiredBones();
    // A game does not tick the animation at initialization (UAnimationSettings::bTickAnimationOnSkeletalMeshInit is
    // off): the reference pose until the first tick.
    const bool bInitializedAnimInstance = InitializeAnimScriptInstance(bForceReinit, true);
    if (LeaderPoseComponent.IsValid() || (!bInitializedAnimInstance && AnimScriptInstance != nullptr)) return;
    BoneSpaceTransforms = Mesh->GetRefSkeleton().GetRefBonePose();
    FAnimationRuntime::FillUpComponentSpaceTransforms(Mesh->GetRefSkeleton(), BoneSpaceTransforms, GetEditableComponentSpaceTransforms());
    bNeedToFlipSpaceBaseBuffers = true;
    FlipEditableSpaceBases();
}

bool USkeletalMeshComponent::InitializeAnimScriptInstance(bool bForceReinit, bool bInDeferRootNodeInitialization) {
    if (!IsRegistered()) return false;
    USkeletalMesh* SkelMesh = GetSkeletalMeshAsset();
    bool bInitializedMainInstance = false;
    bool bInitializedPostInstance = false;
    if (NeedToSpawnAnimScriptInstance()) {
        AnimScriptInstance = NewObject<UAnimInstance>(this, AnimClass);
        // The linked instances belong to the previous main instance.
        ResetLinkedAnimInstances();
        AnimScriptInstance->InitializeAnimation(bInDeferRootNodeInitialization);
        bInitializedMainInstance = true;
    } else if (SkelMesh->GetSkeleton() != nullptr && AnimationMode == EAnimationMode::AnimationSingleNode) {
        UAnimSingleNodeInstance* OldInstance = bForceReinit ? nullptr : Cast<UAnimSingleNodeInstance>(AnimScriptInstance);
        auto* NewInstance = NewObject<UAnimSingleNodeInstance>(this);
        AnimScriptInstance = NewInstance;
        ResetLinkedAnimInstances();
        AnimScriptInstance->InitializeAnimation(bInDeferRootNodeInitialization);
        bInitializedMainInstance = true;
        // The old instance's play state carries over; a new one starts from AnimationData.
        if (OldInstance != nullptr) {
            FSingleAnimationPlayData CachedData;
            CachedData.PopulateFrom(OldInstance);
            CachedData.Initialize(NewInstance);
        } else {
            AnimationData.Initialize(NewInstance);
        }
    }

    UClass* NewMeshInstanceClass = SkelMesh->GetPostProcessAnimBlueprint();
    if (NewMeshInstanceClass == nullptr || NewMeshInstanceClass == AnimClass ||
        (PostProcessAnimInstance != nullptr && PostProcessAnimInstance->CurrentSkeleton != SkelMesh->GetSkeleton()) || SkelMesh->GetSkeleton() == nullptr)
        PostProcessAnimInstance = nullptr;
    if (NeedToSpawnPostPhysicsInstance(bForceReinit)) {
        PostProcessAnimInstance = NewObject<UAnimInstance>(this, NewMeshInstanceClass);
        PostProcessAnimInstance->InitializeAnimation();
        if (FAnimNode_LinkedInputPose* InputNode = PostProcessAnimInstance->GetLinkedInputPoseNode()) {
            InputNode->CachedInputPose.SetBoneContainer(&PostProcessAnimInstance->GetProxyOnGameThread().GetRequiredBones());
            InputNode->bIsCachedInputPoseInitialized = false;
        }
        bInitializedPostInstance = true;
    }

    if (AnimScriptInstance != nullptr && !bInitializedMainInstance && bForceReinit) {
        ResetLinkedAnimInstances();
        AnimScriptInstance->InitializeAnimation(bInDeferRootNodeInitialization);
        bInitializedMainInstance = true;
    }
    if (PostProcessAnimInstance != nullptr && !bInitializedPostInstance && bForceReinit) {
        PostProcessAnimInstance->InitializeAnimation();
        bInitializedPostInstance = true;
    }
    return bInitializedMainInstance || bInitializedPostInstance;
}

void USkeletalMeshComponent::ClearAnimScriptInstance() {
    HandleExistingParallelEvaluationTask(true, false);
    if (AnimScriptInstance != nullptr) AnimScriptInstance->EndNotifyStates();
    AnimScriptInstance = nullptr;
    ResetLinkedAnimInstances();
}

void USkeletalMeshComponent::ResetLinkedAnimInstances() {
    for (UAnimInstance* LinkedInstance : LinkedInstances)
        if (LinkedInstance != nullptr && LinkedInstance->bCreatedByLinkedAnimGraph) LinkedInstance->EndNotifyStates();
    LinkedInstances.Reset();
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const { return Cast<UAnimSingleNodeInstance>(AnimScriptInstance); }

void USkeletalMeshComponent::SetAnimInstanceClass(UClass* NewClass) {
    if (NewClass == nullptr) {
        AnimClass = nullptr;
        ClearAnimScriptInstance();
        return;
    }
    const bool bWasUsingBlueprintMode = AnimationMode == EAnimationMode::AnimationBlueprint;
    AnimationMode = EAnimationMode::AnimationBlueprint;
    if (NewClass == AnimClass && bWasUsingBlueprintMode) return;
    AnimClass = NewClass;
    ClearAnimScriptInstance();
    InitAnim(true);
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode::Type InAnimationMode, bool bForceInitAnimScriptInstance) {
    const bool bNeedChange = AnimationMode != InAnimationMode;
    if (bNeedChange) {
        AnimationMode = InAnimationMode;
        ClearAnimScriptInstance();
    }
    if (GetSkeletalMeshAsset() != nullptr && (bNeedChange || (AnimationMode == EAnimationMode::AnimationBlueprint && bForceInitAnimScriptInstance)))
        InitializeAnimScriptInstance(true);
}

namespace {

// UE logs a warning for a single node call on a component in another mode, and does nothing.
void ReportNotSingleNode(const USkeletalMeshComponent* Component) {
    if (Component->GetAnimInstance() != nullptr) ReportLoadProblem("anim.not_single_node", Component->GetPathName());
}

}

void USkeletalMeshComponent::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping) {
    SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetAnimation(NewAnimToPlay);
    Play(bLooping);
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay) {
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance == nullptr) return ReportNotSingleNode(this);
    SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
    SingleNodeInstance->SetPlaying(false);
}

void USkeletalMeshComponent::Play(bool bLooping) {
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance == nullptr) return ReportNotSingleNode(this);
    SingleNodeInstance->SetPlaying(true);
    SingleNodeInstance->SetLooping(bLooping);
}

void USkeletalMeshComponent::Stop() {
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance == nullptr) return ReportNotSingleNode(this);
    SingleNodeInstance->SetPlaying(false);
}

bool USkeletalMeshComponent::IsPlaying() const {
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance()) return SingleNodeInstance->IsPlaying();
    ReportNotSingleNode(this);
    return false;
}

void USkeletalMeshComponent::SetPosition(float InPos, bool bFireNotifies) {
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance == nullptr) return ReportNotSingleNode(this);
    SingleNodeInstance->SetPosition(InPos, bFireNotifies);
}

float USkeletalMeshComponent::GetPosition() const {
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance()) return SingleNodeInstance->GetCurrentTime();
    ReportNotSingleNode(this);
    return 0.f;
}

void USkeletalMeshComponent::SetPlayRate(float Rate) {
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance == nullptr) return ReportNotSingleNode(this);
    SingleNodeInstance->SetPlayRate(Rate);
}

float USkeletalMeshComponent::GetPlayRate() const {
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance()) return SingleNodeInstance->GetPlayRate();
    ReportNotSingleNode(this);
    return 0.f;
}

void USkeletalMeshComponent::OverrideAnimationData(UAnimationAsset* InAnimToPlay, bool bIsLooping, bool bIsPlaying, float Position, float PlayRate) {
    AnimationData.AnimToPlay = InAnimToPlay;
    AnimationData.bSavedLooping = bIsLooping;
    AnimationData.bSavedPlaying = bIsPlaying;
    AnimationData.SavedPosition = Position;
    AnimationData.SavedPlayRate = PlayRate;
    SetAnimationMode(EAnimationMode::AnimationSingleNode);
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance()) AnimationData.Initialize(SingleNodeInstance);
}

void USkeletalMeshComponent::SetDisablePostProcessBlueprint(bool bInDisablePostProcess) {
    if (!bInDisablePostProcess && bDisablePostProcessBlueprint && PostProcessAnimInstance != nullptr) PostProcessAnimInstance->InitializeAnimation();
    bDisablePostProcessBlueprint = bInDisablePostProcess;
}

bool USkeletalMeshComponent::ShouldUpdatePostProcessInstance() const {
    const USkeletalMesh* SkelMesh = GetSkeletalMeshAsset();
    if (SkelMesh != nullptr && !SkelMesh->ShouldEvaluatePostProcessAnimBP(GetPredictedLODLevel())) return false;
    return PostProcessAnimInstance != nullptr && !bDisablePostProcessBlueprint;
}

bool USkeletalMeshComponent::ShouldEvaluatePostProcessInstance() const { return ShouldUpdatePostProcessInstance(); }

void USkeletalMeshComponent::TickAnimation(float DeltaTime) {
    if (GetSkeletalMeshAsset() == nullptr || bPauseAnims || bNoSkeletonUpdate) return;
    bNeedsQueuedAnimEventsDispatched = true;
    // The linked instances first: their game thread part runs before the main graph updates them (UE's
    // TickAnimInstances). A linked instance can link others, so the array is read by index.
    for (int32 Index = 0; Index < LinkedInstances.Num(); ++Index) LinkedInstances[Index]->UpdateAnimation(DeltaTime * GlobalAnimRateScale);
    if (AnimScriptInstance != nullptr) AnimScriptInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale);
    if (ShouldUpdatePostProcessInstance()) PostProcessAnimInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale);
}

void USkeletalMeshComponent::EvaluatePostProcessMeshInstance(FCompactPose& InOutPose, FBlendedCurve& InOutCurve) const {
    if (!ShouldEvaluatePostProcessInstance() || !PostProcessAnimInstance->ParallelCanEvaluate(GetSkeletalMeshAsset())) return;
    // The main pose is the post process graph's input.
    FAnimInstanceProxy& Proxy = PostProcessAnimInstance->GetProxyOnAnyThread();
    if (FAnimNode_LinkedInputPose* InputNode = PostProcessAnimInstance->GetLinkedInputPoseNode()) {
        if (InOutPose.IsValid()) {
            InputNode->CachedInputPose.CopyBonesFrom(InOutPose);
            InputNode->CachedInputCurve.CopyFrom(InOutCurve);
        } else {
            InputNode->CachedInputPose.SetBoneContainer(&Proxy.GetRequiredBones());
            InputNode->CachedInputPose.ResetToRefPose();
            InputNode->CachedInputCurve = FBlendedCurve();
        }
        InputNode->bIsCachedInputPoseInitialized = true;
    }
    FPoseContext Output(&Proxy);
    PostProcessAnimInstance->ParallelEvaluateAnimation(Output);
    InOutPose.MoveBonesFrom(Output.Pose);
    InOutCurve.CopyFrom(Output.Curve);
}

void USkeletalMeshComponent::PerformAnimationProcessing(const USkeletalMesh* InSkeletalMesh, UAnimInstance* InAnimInstance, bool bInDoEvaluation,
                                                        TArray<FTransform>& OutSpaceBases, TArray<FTransform>& OutBoneSpaceTransforms,
                                                        FBlendedCurve& OutCurve) const {
    if (InSkeletalMesh == nullptr) return;
    if (InAnimInstance != nullptr && InAnimInstance->NeedsUpdate()) InAnimInstance->ParallelUpdateAnimation();
    if (ShouldUpdatePostProcessInstance()) PostProcessAnimInstance->ParallelUpdateAnimation();
    if (!bInDoEvaluation) return;
    FCompactPose EvaluatedPose;
    if (InAnimInstance != nullptr && InSkeletalMesh->GetSkeleton() != nullptr && InAnimInstance->ParallelCanEvaluate(InSkeletalMesh)) {
        FPoseContext Output(&InAnimInstance->GetProxyOnAnyThread());
        if (bForceRefpose) Output.ResetToRefPose();
        else InAnimInstance->ParallelEvaluateAnimation(Output);
        EvaluatedPose.MoveBonesFrom(Output.Pose);
        OutCurve.CopyFrom(Output.Curve);
    }
    EvaluatePostProcessMeshInstance(EvaluatedPose, OutCurve);
    // UE's FinalizePoseEvaluationResult: the compact pose covers the required bones; the others keep their reference
    // transform.
    OutBoneSpaceTransforms = InSkeletalMesh->GetRefSkeleton().GetRefBonePose();
    if (EvaluatedPose.IsValid() && EvaluatedPose.GetNumBones() > 0) {
        EvaluatedPose.NormalizeRotations();
        const FBoneContainer& Bones = EvaluatedPose.GetBoneContainer();
        for (int32 Index = 0; Index < EvaluatedPose.GetNumBones(); ++Index) {
            const FCompactPoseBoneIndex BoneIndex(Index);
            OutBoneSpaceTransforms[Bones.MakeMeshPoseIndex(BoneIndex).GetInt()] = EvaluatedPose[BoneIndex];
        }
    }
    FAnimationRuntime::FillUpComponentSpaceTransforms(InSkeletalMesh->GetRefSkeleton(), OutBoneSpaceTransforms, OutSpaceBases);
}

void USkeletalMeshComponent::ParallelAnimationEvaluation() {
    PerformAnimationProcessing(AnimEvaluationContext.SkeletalMesh, AnimEvaluationContext.AnimInstance, AnimEvaluationContext.bDoEvaluation,
                               AnimEvaluationContext.ComponentSpaceTransforms, AnimEvaluationContext.BoneSpaceTransforms, AnimEvaluationContext.Curve);
}

void USkeletalMeshComponent::DoInstancePreEvaluation() {
    if (AnimScriptInstance != nullptr) {
        AnimScriptInstance->PreEvaluateAnimation();
        for (UAnimInstance* LinkedInstance : LinkedInstances) LinkedInstance->PreEvaluateAnimation();
    }
    if (ShouldEvaluatePostProcessInstance()) PostProcessAnimInstance->PreEvaluateAnimation();
}

void USkeletalMeshComponent::DoInstancePostEvaluation() {
    if (AnimScriptInstance != nullptr) {
        AnimScriptInstance->PostEvaluateAnimation();
        for (UAnimInstance* LinkedInstance : LinkedInstances) LinkedInstance->PostEvaluateAnimation();
    }
    if (PostProcessAnimInstance != nullptr) PostProcessAnimInstance->PostEvaluateAnimation();
}

void USkeletalMeshComponent::RefreshBoneTransforms(FActorComponentTickFunction* TickFunction) {
    const USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (Mesh == nullptr || (AnimScriptInstance == nullptr && PostProcessAnimInstance == nullptr) || bNoSkeletonUpdate) return;
    const bool bMainInstanceValidForParallelWork = AnimScriptInstance == nullptr || AnimScriptInstance->CanRunParallelWork();
    const bool bPostInstanceValidForParallelWork = PostProcessAnimInstance == nullptr || PostProcessAnimInstance->CanRunParallelWork();
    const bool bDoParallelEvaluation = bMainInstanceValidForParallelWork && bPostInstanceValidForParallelWork && TickFunction != nullptr;
    // Without parallel evaluation, a running task completes first.
    if (HandleExistingParallelEvaluationTask(!bDoParallelEvaluation, true)) return;

    AnimEvaluationContext.SkeletalMesh = Mesh;
    AnimEvaluationContext.AnimInstance = AnimScriptInstance;
    AnimEvaluationContext.PostProcessAnimInstance = ShouldEvaluatePostProcessInstance() ? PostProcessAnimInstance : nullptr;
    AnimCurves.Empty();
    AnimEvaluationContext.bDoEvaluation = true;
    // An instance that has never updated, and does not update in this evaluation, ticks once first.
    bool bShouldTickAnimation = AnimScriptInstance != nullptr && !AnimScriptInstance->NeedsUpdate() &&
                                !AnimScriptInstance->GetProxyOnGameThread().GetUpdateCounter().HasEverBeenUpdated();
    bShouldTickAnimation = bShouldTickAnimation ||
                           (ShouldUpdatePostProcessInstance() && !PostProcessAnimInstance->GetProxyOnGameThread().GetUpdateCounter().HasEverBeenUpdated());
    if (bShouldTickAnimation) TickAnimation(0.f);
    DoInstancePreEvaluation();

    if (bDoParallelEvaluation) {
        DispatchParallelEvaluationTasks(TickFunction);
        return;
    }
    DoParallelEvaluationTasks_OnGameThread();
    PostAnimEvaluation(AnimEvaluationContext);
    AnimEvaluationContext.Clear();
}

void USkeletalMeshComponent::SwapEvaluationContextBuffers() {
    std::swap(AnimEvaluationContext.ComponentSpaceTransforms, GetEditableComponentSpaceTransforms());
    std::swap(AnimEvaluationContext.BoneSpaceTransforms, BoneSpaceTransforms);
    std::swap(AnimEvaluationContext.Curve, AnimCurves);
}

void USkeletalMeshComponent::DispatchParallelEvaluationTasks(FActorComponentTickFunction* TickFunction) {
    SwapEvaluationContextBuffers();
    if (ParallelAnimationEvaluationTask == nullptr) ParallelAnimationEvaluationTask = std::make_unique<FParallelAnimationEvaluationTask>();
    bRunningParallelEvaluation = true;
    FParallelAnimationEvaluationTask::Arena().execute([this] {
        ParallelAnimationEvaluationTask->Group.run([this] {
            const FParallelAnimationEvaluationScope Scope;
            ParallelAnimationEvaluation();
        });
    });
    // UE's FParallelAnimationCompletionTask: the game thread takes the result before the tick completes.
    TickFunction->DontCompleteUntil([this] { HandleExistingParallelEvaluationTask(true, true); });
}

void USkeletalMeshComponent::DoParallelEvaluationTasks_OnGameThread() {
    SwapEvaluationContextBuffers();
    ParallelAnimationEvaluation();
    SwapEvaluationContextBuffers();
}

void USkeletalMeshComponent::CompleteParallelAnimationEvaluation(bool bDoPostAnimEvaluation) {
    bRunningParallelEvaluation = false;
    if (bDoPostAnimEvaluation && AnimEvaluationContext.AnimInstance == AnimScriptInstance && AnimEvaluationContext.SkeletalMesh == GetSkeletalMeshAsset()) {
        SwapEvaluationContextBuffers();
        PostAnimEvaluation(AnimEvaluationContext);
    }
    AnimEvaluationContext.Clear();
}

bool USkeletalMeshComponent::HandleExistingParallelEvaluationTask(bool bBlockOnTask, bool bPerformPostAnimEvaluation) {
    if (!IsRunningParallelEvaluation()) return false;
    if (!bBlockOnTask) return true;
    // A task that waited on itself would never complete (UE checks IsInGameThread here).
    UE5PARSE_CHECK(!FParallelAnimationEvaluationScope::IsActive());
    FParallelAnimationEvaluationTask::Arena().execute([this] { ParallelAnimationEvaluationTask->Group.wait(); });
    CompleteParallelAnimationEvaluation(bPerformPostAnimEvaluation);
    return true;
}

void USkeletalMeshComponent::PostAnimEvaluation(FAnimationEvaluationContext& EvaluationContext) {
    if (EvaluationContext.AnimInstance != nullptr) EvaluationContext.AnimInstance->PostUpdateAnimation();
    if (ShouldUpdatePostProcessInstance()) PostProcessAnimInstance->PostUpdateAnimation();
    // A notify or an event can unregister the component.
    if (!IsRegistered()) return;
    if (AnimScriptInstance != nullptr) {
        AnimScriptInstance->SetAnimationCurves(AnimCurves);
        for (UAnimInstance* LinkedInstance : LinkedInstances) LinkedInstance->CopyCurveValues(*AnimScriptInstance);
    }
    if (PostProcessAnimInstance != nullptr) {
        if (AnimScriptInstance != nullptr) PostProcessAnimInstance->CopyCurveValues(*AnimScriptInstance);
        else PostProcessAnimInstance->SetAnimationCurves(AnimCurves);
    }
    if (EvaluationContext.bDoEvaluation) DoInstancePostEvaluation();
    bNeedToFlipSpaceBaseBuffers = true;
    FinalizeAnimationUpdate();
}

void USkeletalMeshComponent::ConditionallyDispatchQueuedAnimEvents() {
    if (!bNeedsQueuedAnimEventsDispatched) return;
    bNeedsQueuedAnimEventsDispatched = false;
    // A notify can change the linked instances.
    const TArray<UAnimInstance*> Linked = LinkedInstances;
    for (UAnimInstance* LinkedInstance : Linked) LinkedInstance->DispatchQueuedAnimEvents();
    if (AnimScriptInstance != nullptr) AnimScriptInstance->DispatchQueuedAnimEvents();
    if (PostProcessAnimInstance != nullptr) PostProcessAnimInstance->DispatchQueuedAnimEvents();
}

void USkeletalMeshComponent::FinalizeAnimationUpdate() {
    FinalizeBoneTransform();
    UpdateChildTransforms();
}

void USkeletalMeshComponent::FinalizeBoneTransform() {
    FlipEditableSpaceBases();
    bHasValidBoneTransform = true;
    // The events of the frame after its pose, so that they read the new bones.
    ConditionallyDispatchQueuedAnimEvents();
}

void USkeletalMeshComponent::FlipEditableSpaceBases() {
    if (!bNeedToFlipSpaceBaseBuffers) return;
    bNeedToFlipSpaceBaseBuffers = false;
    CurrentReadComponentTransforms = CurrentEditableComponentTransforms;
    CurrentEditableComponentTransforms = 1 - CurrentEditableComponentTransforms;
    // The first pose goes to both buffers.
    if (!bHasValidBoneTransform) GetEditableComponentSpaceTransforms() = GetComponentSpaceTransforms();
}

}
