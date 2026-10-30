#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_LookAt.h"

#include "ue5parse/runtime/AnimationCore/AnimationCoreLibrary.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

static const FVector DefaultLookAtAxis(0.0, 1.0, 0.0);
static const FVector DefaultLookUpAxis(1.0, 0.0, 0.0);

// A linear alpha turned into the curve of a blend type.
static float AlphaToBlendType(float InAlpha, EInterpolationBlend::Type BlendType) {
    switch (BlendType) {
        case EInterpolationBlend::Sinusoidal: return FMath::Clamp<float>((FMath::Sin(InAlpha * UE_PI - UE_HALF_PI) + 1.f) / 2.f, 0.f, 1.f);
        case EInterpolationBlend::Cubic: return FMath::Clamp<float>(FMath::CubicInterp<float>(0.f, 0.f, 1.f, 0.f, InAlpha), 0.f, 1.f);
        case EInterpolationBlend::EaseInOutExponent2: return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 2), 0.f, 1.f);
        case EInterpolationBlend::EaseInOutExponent3: return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 3), 0.f, 1.f);
        case EInterpolationBlend::EaseInOutExponent4: return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 4), 0.f, 1.f);
        case EInterpolationBlend::EaseInOutExponent5: return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 5), 0.f, 1.f);
        default: return InAlpha;
    }
}

void FAnimNode_LookAt::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FCompactPoseBoneIndex ModifyBoneIndex = BoneToModify.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
    FTransform ComponentBoneTransform = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex);
    const FVector NewCurrentTargetLocation =
        LookAtTarget.GetTargetTransform(LookAtLocation, Output.Pose, Output.AnimInstanceProxy->GetComponentTransform()).GetLocation();
    const FVector OldCurrentTargetLocation = CurrentTargetLocation;
    if ((NewCurrentTargetLocation - OldCurrentTargetLocation).SizeSquared() > InterpolationTriggerThreashold * InterpolationTriggerThreashold) {
        // A new target starts a new interpolation, unless one is in progress.
        if (AccumulatedInterpoolationTime >= InterpolationTime) AccumulatedInterpoolationTime = 0.f;
        PreviousTargetLocation = OldCurrentTargetLocation;
        CurrentTargetLocation = NewCurrentTargetLocation;
    } else if (InterpolationTriggerThreashold == 0.f) {
        CurrentTargetLocation = NewCurrentTargetLocation;
    }
    if (InterpolationTime <= 0.f) {
        CurrentLookAtLocation = CurrentTargetLocation;
    } else if (const float CurrentAlpha = AccumulatedInterpoolationTime / InterpolationTime; CurrentAlpha < 1.f) {
        CurrentLookAtLocation = FMath::Lerp(PreviousTargetLocation, CurrentTargetLocation, double(AlphaToBlendType(CurrentAlpha, InterpolationType)));
    }
    const FVector LookAtVector = LookAt_Axis.GetTransformedAxis(ComponentBoneTransform);
    const FVector LookUpVector = LookUp_Axis.GetTransformedAxis(ComponentBoneTransform);
    const FQuat DeltaRotation =
        AnimationCore::SolveAim(ComponentBoneTransform, CurrentLookAtLocation, LookAtVector, bUseLookUpAxis, LookUpVector, LookAtClamp);
    ComponentBoneTransform.SetRotation(DeltaRotation * ComponentBoneTransform.GetRotation());
    OutBoneTransforms.Add(FBoneTransform(ModifyBoneIndex, ComponentBoneTransform));
}

// A target that is set but not in the bone container does not stop the look at; the target location is then a world
// location.
bool FAnimNode_LookAt::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return BoneToModify.IsValidToEvaluate(RequiredBones) && (!LookAtTarget.HasTargetSetup() || LookAtTarget.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_LookAt::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    BoneToModify.Initialize(RequiredBones);
    LookAtTarget.InitializeBoneReferences(RequiredBones);
}

void FAnimNode_LookAt::UpdateInternal(const FAnimationUpdateContext& Context) {
    AccumulatedInterpoolationTime = FMath::Clamp(AccumulatedInterpoolationTime + Context.GetDeltaTime(), 0.f, InterpolationTime);
}

void FAnimNode_LookAt::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
    LookAtTarget.Initialize(Context.AnimInstanceProxy);
    // UE reverts a zero axis to the default with a warning.
    LookUp_Axis.Initialize();
    if (LookUp_Axis.Axis.IsZero()) {
        ReportLoadProblem("anim.look_at_zero_axis", Context.GetAnimInstanceObject()->GetPathName());
        LookUp_Axis.Axis = DefaultLookUpAxis;
    }
    LookAt_Axis.Initialize();
    if (LookAt_Axis.Axis.IsZero()) {
        ReportLoadProblem("anim.look_at_zero_axis", Context.GetAnimInstanceObject()->GetPathName());
        LookAt_Axis.Axis = DefaultLookAtAxis;
    }
}

}
