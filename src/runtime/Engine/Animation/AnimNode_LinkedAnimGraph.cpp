#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedAnimGraph.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedInputPose.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_Root.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

void FAnimNode_LinkedAnimGraph::InitializeSubGraph_AnyThread(const FAnimationInitializeContext& Context) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun == nullptr || LinkedRoot == nullptr) return;
    FAnimInstanceProxy& Proxy = InstanceToRun->GetProxyOnAnyThread();
    Proxy.InitializeObjects(InstanceToRun);
    Proxy.InitializationCounter.SynchronizeWith(Context.AnimInstanceProxy->GetInitializationCounter());
    Proxy.InitializeRootNode_WithRoot(LinkedRoot);
}

void FAnimNode_LinkedAnimGraph::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_Base::Initialize_AnyThread(Context);
    InitializeSubGraph_AnyThread(Context);
    for (FPoseLink& InputPose : InputPoses) InputPose.Initialize(Context);
}

void FAnimNode_LinkedAnimGraph::CacheBonesSubGraph_AnyThread(const FAnimationCacheBonesContext& Context) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun == nullptr || LinkedRoot == nullptr) return;
    FAnimInstanceProxy& Proxy = InstanceToRun->GetProxyOnAnyThread();
    Proxy.CachedBonesCounter.SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());
    FAnimationCacheBonesContext LinkedContext(&Proxy);
    LinkedRoot->CacheBones_AnyThread(LinkedContext);
}

void FAnimNode_LinkedAnimGraph::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    CacheBonesSubGraph_AnyThread(Context);
    for (FPoseLink& InputPose : InputPoses) InputPose.CacheBones(Context);
}

void FAnimNode_LinkedAnimGraph::Update_AnyThread(const FAnimationUpdateContext& InContext) {
    GetEvaluateGraphExposedInputs().Execute(InContext);
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun != nullptr && LinkedRoot != nullptr) {
        FAnimInstanceProxy& Proxy = InstanceToRun->GetProxyOnAnyThread();
        Proxy.UpdateCounter.SynchronizeWith(InContext.AnimInstanceProxy->GetUpdateCounter());
        // The inputs of the node are copied to the linked instance before its update.
        PropagateInputProperties(InContext.AnimInstanceProxy->GetAnimInstanceObject());
        const FAnimationUpdateContext NewContext = InContext.WithOtherProxy(&Proxy);
        Proxy.UpdateAnimation_WithRoot(NewContext, LinkedRoot, GetDynamicLinkFunctionName());
    } else if (InputPoses.Num() > 0) {
        InputPoses[0].Update(InContext);
    }
    // A class change asks for an inertial blend; with no inertialization node above this one it pops, as in UE.
    PendingBlendOutDuration = -1.0f;
    PendingBlendOutProfile = nullptr;
    PendingBlendInDuration = -1.0f;
    PendingBlendInProfile = nullptr;
}

void FAnimNode_LinkedAnimGraph::Evaluate_AnyThread(FPoseContext& Output) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun != nullptr && LinkedRoot != nullptr) {
        FAnimInstanceProxy& Proxy = InstanceToRun->GetProxyOnAnyThread();
        Proxy.EvaluationCounter.SynchronizeWith(Output.AnimInstanceProxy->GetEvaluationCounter());
        Output.Pose.SetBoneContainer(&Proxy.GetRequiredBones());
        FPoseContext EvaluationContext(&Proxy, Output.ExpectsAdditivePose());
        EvaluationContext.ResetToRefPose();
        Proxy.EvaluateAnimation_WithRoot(EvaluationContext, LinkedRoot);
        Output.Curve.CopyFrom(EvaluationContext.Curve);
        Output.Pose.MoveBonesFrom(EvaluationContext.Pose);
    } else if (InputPoses.Num() > 0) {
        InputPoses[0].Evaluate(Output);
    } else {
        Output.ResetToRefPose();
    }
}

void FAnimNode_LinkedAnimGraph::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceClass.Get() != nullptr) ReinitializeLinkedAnimInstance(InAnimInstance);
    else if (InstanceToRun != nullptr) TeardownInstance(InAnimInstance);
    InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun != nullptr) InstanceToRun->GetProxyOnAnyThread().InitializeObjects(InstanceToRun);
}

void FAnimNode_LinkedAnimGraph::TeardownInstance(const UAnimInstance* InOwningAnimInstance) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    if (InstanceToRun != nullptr) {
        DynamicUnlink(const_cast<UAnimInstance*>(InOwningAnimInstance));
        // A linked layer can run on its owning instance, which is not torn down.
        if (InstanceToRun != InOwningAnimInstance && CanTeardownLinkedInstance(InstanceToRun)) {
            USkeletalMeshComponent* MeshComp = InOwningAnimInstance->GetSkelMeshComponent();
            MeshComp->GetLinkedAnimInstances().Remove(InstanceToRun);
            InstanceToRun->UninitializeAnimation();
        }
    }
    SetTargetInstance(nullptr);
}

void FAnimNode_LinkedAnimGraph::ReinitializeLinkedAnimInstance(const UAnimInstance* InOwningAnimInstance, UAnimInstance* InNewAnimInstance) {
    UAnimInstance* InstanceToRun = GetTargetInstance<UAnimInstance>();
    const auto* PriorAnimBPClass = InstanceToRun ? Cast<UAnimBlueprintGeneratedClass>(InstanceToRun->GetClass()) : nullptr;
    TeardownInstance(InOwningAnimInstance);
    if (InstanceClass.Get() == nullptr && InNewAnimInstance == nullptr) {
        RequestBlend(PriorAnimBPClass, nullptr);
        return;
    }
    USkeletalMeshComponent* MeshComp = InOwningAnimInstance->GetSkelMeshComponent();
    InstanceToRun = InNewAnimInstance ? InNewAnimInstance : NewObject<UAnimInstance>(MeshComp, InstanceClass.Get());
    if (InNewAnimInstance == nullptr) {
        InstanceToRun->bCreatedByLinkedAnimGraph = true;
        InstanceToRun->bPropagateNotifiesToLinkedInstances = bPropagateNotifiesToLinkedInstances;
        InstanceToRun->bReceiveNotifiesFromLinkedInstances = bReceiveNotifiesFromLinkedInstances;
    }
    SetTargetInstance(InstanceToRun);
    // Before the instance initializes, so that its input poses are linked when it initializes its graph.
    DynamicLink(const_cast<UAnimInstance*>(InOwningAnimInstance));
    if (InNewAnimInstance == nullptr) {
        InstanceToRun->InitializeAnimation();
        MeshComp->GetLinkedAnimInstances().Add(InstanceToRun);
    }
    InitializeProperties(InOwningAnimInstance, InstanceToRun->GetClass());
    RequestBlend(PriorAnimBPClass, Cast<UAnimBlueprintGeneratedClass>(InstanceToRun->GetClass()));
}

void FAnimNode_LinkedAnimGraph::SetAnimClass(TSubclassOf<UAnimInstance> InClass, const UAnimInstance* InOwningAnimInstance) {
    const TSubclassOf<UAnimInstance> OldClass = InstanceClass;
    InstanceClass = InClass;
    if (!(InstanceClass == OldClass)) ReinitializeLinkedAnimInstance(InOwningAnimInstance);
}

FName FAnimNode_LinkedAnimGraph::GetDynamicLinkFunctionName() const { return FName("AnimGraph"); }

UAnimInstance* FAnimNode_LinkedAnimGraph::GetDynamicLinkTarget(UAnimInstance* InOwningAnimInstance) const {
    return GetTargetInstance<UAnimInstance>();
}

void FAnimNode_LinkedAnimGraph::DynamicLink(UAnimInstance* InOwningAnimInstance) {
    CachedLinkedNodeIndex = INDEX_NONE;
    UAnimInstance* LinkTargetInstance = GetDynamicLinkTarget(InOwningAnimInstance);
    if (LinkTargetInstance == nullptr) return;
    const auto* SubAnimBlueprintClass = Cast<UAnimBlueprintGeneratedClass>(LinkTargetInstance->GetClass());
    if (SubAnimBlueprintClass == nullptr) return;
    FAnimInstanceProxy* NonConstProxy = &InOwningAnimInstance->GetProxyOnAnyThread();
    const FName FunctionToLink = GetDynamicLinkFunctionName();
    for (const FAnimBlueprintFunction& AnimBlueprintFunction : SubAnimBlueprintClass->GetAnimBlueprintFunctions()) {
        if (AnimBlueprintFunction.Name != FunctionToLink) continue;
        CachedLinkedNodeIndex = AnimBlueprintFunction.OutputPoseNodeIndex;
        // Each input pose of the linked graph runs this node's input of its index, in this instance.
        for (int32 InputPoseIndex = 0; InputPoseIndex < AnimBlueprintFunction.InputPoseNames.Num() && InputPoseIndex < InputPoses.Num(); ++InputPoseIndex) {
            FAnimationInitializeContext Context(NonConstProxy);
            InputPoses[InputPoseIndex].AttemptRelink(Context);
            const int32 InputPropertyIndex = FindFunctionInputIndex(AnimBlueprintFunction, AnimBlueprintFunction.InputPoseNames[InputPoseIndex]);
            if (InputPropertyIndex == INDEX_NONE || AnimBlueprintFunction.InputPoseNodeProperties[InputPropertyIndex] == nullptr) {
                ReportLoadProblem("anim.linked_input_pose", FString(AnimBlueprintFunction.InputPoseNames[InputPoseIndex].toString()));
                continue;
            }
            auto* LinkedInputPoseNode =
                AnimBlueprintFunction.InputPoseNodeProperties[InputPropertyIndex]->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(LinkTargetInstance);
            LinkedInputPoseNode->DynamicLink(NonConstProxy, &InputPoses[InputPoseIndex], NodeIndex);
        }
        if (AnimBlueprintFunction.OutputPoseNodeProperty != nullptr)
            LinkedRoot = AnimBlueprintFunction.OutputPoseNodeProperty->ContainerPtrToValuePtr<FAnimNode_Root>(LinkTargetInstance);
        else
            ReportLoadProblem("anim.linked_root", FString(FunctionToLink.toString()));
        break;
    }
}

void FAnimNode_LinkedAnimGraph::DynamicUnlink(UAnimInstance* InOwningAnimInstance) {
    LinkedRoot = nullptr;
    UAnimInstance* LinkTargetInstance = GetDynamicLinkTarget(InOwningAnimInstance);
    const auto* SubAnimBlueprintClass = LinkTargetInstance ? Cast<UAnimBlueprintGeneratedClass>(LinkTargetInstance->GetClass()) : nullptr;
    if (SubAnimBlueprintClass != nullptr) {
        const FName FunctionToLink = GetDynamicLinkFunctionName();
        for (const FAnimBlueprintFunction& AnimBlueprintFunction : SubAnimBlueprintClass->GetAnimBlueprintFunctions()) {
            if (AnimBlueprintFunction.Name != FunctionToLink) continue;
            for (int32 InputPoseIndex = 0; InputPoseIndex < AnimBlueprintFunction.InputPoseNames.Num() && InputPoseIndex < InputPoses.Num();
                 ++InputPoseIndex) {
                const int32 InputPropertyIndex = FindFunctionInputIndex(AnimBlueprintFunction, AnimBlueprintFunction.InputPoseNames[InputPoseIndex]);
                if (InputPropertyIndex == INDEX_NONE || AnimBlueprintFunction.InputPoseNodeProperties[InputPropertyIndex] == nullptr) continue;
                AnimBlueprintFunction.InputPoseNodeProperties[InputPropertyIndex]
                    ->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(LinkTargetInstance)
                    ->DynamicUnlink();
            }
            break;
        }
    }
    CachedLinkedNodeIndex = INDEX_NONE;
}

int32 FAnimNode_LinkedAnimGraph::FindFunctionInputIndex(const FAnimBlueprintFunction& InAnimBlueprintFunction, const FName& InInputName) {
    for (int32 InputPropertyIndex = 0; InputPropertyIndex < InAnimBlueprintFunction.InputPoseNames.Num(); ++InputPropertyIndex)
        if (InInputName == InAnimBlueprintFunction.InputPoseNames[InputPropertyIndex]) return InputPropertyIndex;
    return INDEX_NONE;
}

void FAnimNode_LinkedAnimGraph::RequestBlend(const UAnimBlueprintGeneratedClass* PriorAnimBPClass, const UAnimBlueprintGeneratedClass* NewAnimBPClass) {
    const FName Layer = GetDynamicLinkFunctionName();
    const FAnimGraphBlendOptions* PriorBlendOptions = PriorAnimBPClass ? PriorAnimBPClass->GetGraphBlendOptions().Find(Layer) : nullptr;
    const FAnimGraphBlendOptions* NewBlendOptions = NewAnimBPClass ? NewAnimBPClass->GetGraphBlendOptions().Find(Layer) : nullptr;
    if (PriorBlendOptions && PriorBlendOptions->BlendOutTime >= 0.0f) {
        PendingBlendOutDuration = PriorBlendOptions->BlendOutTime;
        PendingBlendOutProfile = PriorBlendOptions->BlendOutProfile;
    } else {
        PendingBlendOutDuration = -1.0f;
        PendingBlendOutProfile = nullptr;
    }
    if (NewBlendOptions && NewBlendOptions->BlendInTime >= 0.0f) {
        PendingBlendInDuration = NewBlendOptions->BlendInTime;
        PendingBlendInProfile = NewBlendOptions->BlendInProfile;
    } else {
        PendingBlendInDuration = -1.0f;
        PendingBlendInProfile = nullptr;
    }
}

}
