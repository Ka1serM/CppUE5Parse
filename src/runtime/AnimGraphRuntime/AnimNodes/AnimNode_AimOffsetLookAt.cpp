#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_AimOffsetLookAt.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"

namespace ue5parse::runtime {

void FAnimNode_AimOffsetLookAt::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_BlendSpacePlayer::Initialize_AnyThread(Context);
    BasePose.Initialize(Context);
}

// The source and pivot are sockets of the mesh, else bones.
void FAnimNode_AimOffsetLookAt::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) {
    SocketBoneReference.BoneName = FName();
    const USkeletalMeshComponent* SkelMeshComp = InAnimInstance->GetSkelMeshComponent();
    const USkeletalMesh* SkelMesh = SkelMeshComp ? SkelMeshComp->GetSkeletalMeshAsset() : nullptr;
    if (SkelMesh == nullptr) return;
    auto Resolve = [SkelMesh](FName SocketName, FTransform& OutLocalTransform, FBoneReference& OutBoneReference) {
        if (const USkeletalMeshSocket* Socket = SkelMesh->FindSocket(SocketName)) {
            OutLocalTransform = Socket->GetSocketLocalTransform();
            OutBoneReference.BoneName = Socket->BoneName;
        } else if (SkelMesh->GetRefSkeleton().FindBoneIndex(SocketName) != INDEX_NONE) {
            OutLocalTransform.SetIdentity();
            OutBoneReference.BoneName = SocketName;
        }
    };
    Resolve(SourceSocketName, SocketLocalTransform, SocketBoneReference);
    Resolve(PivotSocketName, PivotSocketLocalTransform, PivotSocketBoneReference);
}

void FAnimNode_AimOffsetLookAt::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    bIsLODEnabled = true;
    FAnimNode_BlendSpacePlayer::UpdateAssetPlayer(Context);
    BasePose.Update(Context);
}

void FAnimNode_AimOffsetLookAt::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    FAnimNode_BlendSpacePlayer::CacheBones_AnyThread(Context);
    BasePose.CacheBones(Context);
    SocketBoneReference.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
    PivotSocketBoneReference.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
}

void FAnimNode_AimOffsetLookAt::Evaluate_AnyThread(FPoseContext& Output) {
    BasePose.Evaluate(Output);
    if (!bIsLODEnabled || !FAnimWeight::IsRelevant(Alpha)) return;
    UpdateFromLookAtTarget(Output);
    FPoseContext MeshSpaceRotationAdditivePoseContext(Output);
    FAnimNode_BlendSpacePlayer::Evaluate_AnyThread(MeshSpaceRotationAdditivePoseContext);
    FAnimationPoseData BaseAnimationPoseData = Output.GetAnimationPoseData();
    const FAnimationPoseData AdditiveAnimationPoseData = MeshSpaceRotationAdditivePoseContext.GetAnimationPoseData();
    FAnimationRuntime::AccumulateAdditivePose(BaseAnimationPoseData, AdditiveAnimationPoseData, Alpha, AAT_RotationOffsetMeshSpace);
}

// The input is the target's direction relative to the socket's axis, as azimuth and elevation in the actor's space.
// The evaluation then resamples the blend space at it.
void FAnimNode_AimOffsetLookAt::UpdateFromLookAtTarget(FPoseContext& LocalPoseContext) {
    UBlendSpace* CurrentBlendSpace = GetBlendSpace();
    const FBoneContainer& RequiredBones = LocalPoseContext.Pose.GetBoneContainer();
    if (CurrentBlendSpace != nullptr && SocketBoneReference.IsValidToEvaluate(RequiredBones)) {
        FCSPose<FCompactPose> GlobalPose;
        GlobalPose.InitPose(LocalPoseContext.Pose);
        const FTransform BoneTransform = GlobalPose.GetComponentSpaceTransform(SocketBoneReference.GetCompactPoseIndex(RequiredBones));
        FTransform SourceComponentTransform = SocketLocalTransform * BoneTransform;
        if (PivotSocketBoneReference.IsValidToEvaluate(RequiredBones))
            SourceComponentTransform.SetTranslation(
                GlobalPose.GetComponentSpaceTransform(PivotSocketBoneReference.GetCompactPoseIndex(RequiredBones)).GetTranslation());
        const FAnimInstanceProxy* AnimProxy = LocalPoseContext.AnimInstanceProxy;
        const FTransform SourceWorldTransform = SourceComponentTransform * AnimProxy->GetComponentTransform();
        const FTransform& ActorTransform = AnimProxy->GetActorTransform();
        const FVector DirectionToTarget = ActorTransform.InverseTransformVectorNoScale(LookAtLocation - SourceWorldTransform.GetLocation()).GetSafeNormal();
        const FVector CurrentDirection = ActorTransform.InverseTransformVectorNoScale(SourceWorldTransform.TransformVector(SocketAxis).GetSafeNormal());
        const FVector2D CurrentCoords = FMath::GetAzimuthAndElevation(CurrentDirection, FVector::ForwardVector, FVector::RightVector, FVector::UpVector);
        const FVector2D TargetCoords = FMath::GetAzimuthAndElevation(DirectionToTarget, FVector::ForwardVector, FVector::RightVector, FVector::UpVector);
        CurrentBlendInput.X = FRotator::NormalizeAxis(FMath::RadiansToDegrees(TargetCoords.X - CurrentCoords.X));
        CurrentBlendInput.Y = FRotator::NormalizeAxis(FMath::RadiansToDegrees(TargetCoords.Y - CurrentCoords.Y));
    }
    if (CurrentBlendSpace == nullptr) return;
    const FVector BlendSpacePosition(CurrentBlendInput.X, CurrentBlendInput.Y, 0.f);
    const FVector FilteredBlendInput = CurrentBlendSpace->FilterInput(&BlendFilter, BlendSpacePosition, DeltaTimeRecord.Delta);
    CurrentBlendSpace->UpdateBlendSamples(FilteredBlendInput, DeltaTimeRecord.Delta, BlendSampleDataCache, CachedTriangulationIndex);
}

}
