#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SpringBone.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

void FAnimNode_SpringBone::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
    RemainingTime = 0.0f;
}

void FAnimNode_SpringBone::UpdateInternal(const FAnimationUpdateContext& Context) {
    FAnimNode_SkeletalControlBase::UpdateInternal(Context);
    RemainingTime += Context.GetDeltaTime();
    FixedTimeStep = (1.f / 120.f) * TimeDilation;
}

static void CopyToVectorByFlags(FVector& DestVec, const FVector& SrcVec, bool bX, bool bY, bool bZ) {
    if (bX) DestVec.X = SrcVec.X;
    if (bY) DestVec.Y = SrcVec.Y;
    if (bZ) DestVec.Z = SrcVec.Z;
}

void FAnimNode_SpringBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    if (!bTranslateX && !bTranslateY && !bTranslateZ) return;
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex SpringBoneIndex = SpringBone.GetCompactPoseIndex(BoneContainer);
    const FTransform& SpaceBase = Output.Pose.GetComponentSpaceTransform(SpringBoneIndex);
    const FTransform BoneTransformInWorldSpace = SpaceBase * Output.AnimInstanceProxy->GetComponentTransform();
    const FVector TargetPos = BoneTransformInWorldSpace.GetLocation();

    // The first frame after initialization starts at rest.
    if (RemainingTime == 0.0f) {
        BoneLocation = TargetPos;
        BoneVelocity = FVector::ZeroVector;
    }

    if (!FMath::IsNearlyZero(FixedTimeStep, UE_KINDA_SMALL_NUMBER)) {
        while (RemainingTime > FixedTimeStep) {
            BoneLocation += OwnerVelocity * FixedTimeStep;
            // Too far from the target (a teleport): reset to it.
            if ((TargetPos - BoneLocation).SizeSquared() > ErrorResetThresh * ErrorResetThresh) {
                BoneLocation = TargetPos;
                BoneVelocity = FVector::ZeroVector;
            }
            const FVector Error = TargetPos - BoneLocation;
            const FVector DampingForce = SpringDamping * BoneVelocity;
            const FVector SpringForce = SpringStiffness * Error;
            const FVector Acceleration = SpringForce - DampingForce;
            // A damping above 1 / step would overshoot: it is scaled down.
            const double CutOffDampingValue = 1.0 / FixedTimeStep;
            if (SpringDamping > CutOffDampingValue) {
                const double SafetyScale = CutOffDampingValue / SpringDamping;
                BoneVelocity += SafetyScale * (Acceleration * FixedTimeStep);
            } else {
                BoneVelocity += Acceleration * FixedTimeStep;
            }
            const double BoneVelocityMagnitude = BoneVelocity.Size();
            if (BoneVelocityMagnitude * FixedTimeStep > ErrorResetThresh) BoneVelocity *= ErrorResetThresh / (BoneVelocityMagnitude * FixedTimeStep);
            const FVector OldBoneLocation = BoneLocation;
            BoneLocation += BoneVelocity * FixedTimeStep;
            CopyToVectorByFlags(BoneLocation, TargetPos, !bTranslateX, !bTranslateY, !bTranslateZ);
            if (bLimitDisplacement) {
                const FVector CurrentDisp = BoneLocation - TargetPos;
                if (CurrentDisp.SizeSquared() > FMath::Square(MaxDisplacement)) BoneLocation = TargetPos + MaxDisplacement * CurrentDisp.GetSafeNormal();
            }
            BoneVelocity = (BoneLocation - OldBoneLocation) / FixedTimeStep;
            RemainingTime -= FixedTimeStep;
        }
        LocalBoneTransform = Output.AnimInstanceProxy->GetComponentTransform().InverseTransformPosition(BoneLocation);
    } else {
        BoneLocation = Output.AnimInstanceProxy->GetComponentTransform().TransformPosition(LocalBoneTransform);
    }

    FTransform OutBoneTM = SpaceBase;
    OutBoneTM.SetLocation(LocalBoneTransform);
    if (bRotateX || bRotateY || bRotateZ) {
        const FCompactPoseBoneIndex ParentBoneIndex = Output.Pose.GetPose().GetParentBoneIndex(SpringBoneIndex);
        const FTransform& ParentSpaceBase = Output.Pose.GetComponentSpaceTransform(ParentBoneIndex);
        const FVector ParentToTarget = (TargetPos - ParentSpaceBase.GetLocation()).GetSafeNormal();
        const FVector ParentToCurrent = (BoneLocation - ParentSpaceBase.GetLocation()).GetSafeNormal();
        const FQuat AdditionalRotation = FQuat::FindBetweenNormals(ParentToTarget, ParentToCurrent);
        FVector EulerRot = AdditionalRotation.Euler();
        CopyToVectorByFlags(EulerRot, FVector::ZeroVector, !bRotateX, !bRotateY, !bRotateZ);
        OutBoneTM.SetRotation(FQuat::MakeFromEuler(EulerRot) * OutBoneTM.GetRotation());
    }
    OutBoneTransforms.Add(FBoneTransform(SpringBoneIndex, OutBoneTM));
}

bool FAnimNode_SpringBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return SpringBone.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_SpringBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    SpringBone.Initialize(RequiredBones);
}

void FAnimNode_SpringBone::PreUpdate(const UAnimInstance* InAnimInstance) {
    const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
    if (SkelComp == nullptr) return;
    const UWorld* World = SkelComp->GetWorld();
    if (World == nullptr) return;
    if (const AWorldSettings* WorldSettings = World->GetWorldSettings()) TimeDilation = WorldSettings->GetEffectiveTimeDilation();
    AActor* SkelOwner = SkelComp->GetOwner();
    if (SkelComp->GetAttachParent() != nullptr && SkelOwner == nullptr) {
        SkelOwner = SkelComp->GetAttachParent()->GetOwner();
        OwnerVelocity = SkelOwner->GetVelocity();
    } else {
        OwnerVelocity = FVector::ZeroVector;
    }
}

}
