#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_CopyPoseFromMesh.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

// Custom attributes are not part of the runtime: bCopyCustomAttributes has no effect.

namespace ue5parse::runtime {

void FAnimNode_CopyPoseFromMesh::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    // The first update of the inputs, so that the setup has no frame of delay.
    GetEvaluateGraphExposedInputs().Execute(Context);
}

void FAnimNode_CopyPoseFromMesh::ResetMeshComponent(USkeletalMeshComponent* InMeshComponent, USkeletalMeshComponent* InTargetMeshComponent) {
    USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent;
    if (CurrentMeshComponent == nullptr) {
        if (InMeshComponent != nullptr) ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
        return;
    }
    if (CurrentMeshComponent != InMeshComponent || CurrentMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedSourceMesh ||
        (InTargetMeshComponent != nullptr && InTargetMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedTargetMesh))
        ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
}

void FAnimNode_CopyPoseFromMesh::RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent) {
    if (SourceMeshComponent.IsValid()) {
        ResetMeshComponent(SourceMeshComponent.Get(), TargetMeshComponent);
        return;
    }
    if (!bUseAttachedParent || TargetMeshComponent == nullptr) {
        CurrentlyUsedSourceMeshComponent = nullptr;
        return;
    }
    // The nearest skeletal mesh component up the attachment chain.
    USkeletalMeshComponent* ParentMeshComponent = nullptr;
    for (USceneComponent* AttachParentComp = TargetMeshComponent->GetAttachParent(); AttachParentComp != nullptr;
         AttachParentComp = AttachParentComp->GetAttachParent()) {
        ParentMeshComponent = Cast<USkeletalMeshComponent>(AttachParentComp);
        if (ParentMeshComponent != nullptr) break;
    }
    if (ParentMeshComponent == nullptr) {
        CurrentlyUsedSourceMeshComponent = nullptr;
        return;
    }
    ResetMeshComponent(ParentMeshComponent, TargetMeshComponent);
}

void FAnimNode_CopyPoseFromMesh::PreUpdate(const UAnimInstance* InAnimInstance) {
    RefreshMeshComponent(InAnimInstance->GetSkelMeshComponent());
    USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent;
    if (CurrentMeshComponent == nullptr || CurrentMeshComponent->GetSkeletalMeshAsset() == nullptr) return;
    // A source that follows a leader pose has its bones there.
    if (auto* LeaderPoseComponent = Cast<USkeletalMeshComponent>(CurrentMeshComponent->LeaderPoseComponent.Get()))
        CurrentMeshComponent = LeaderPoseComponent;
    if (CurrentMeshComponent->GetSkeletalMeshAsset() == nullptr) {
        CurrentlyUsedMesh = nullptr;
        return;
    }
    SourceMeshTransformArray = CurrentMeshComponent->GetComponentSpaceTransforms();
    CurrentlyUsedMesh = CurrentMeshComponent->GetSkeletalMeshAsset();
    if (!bCopyCurves) return;
    const UAnimInstance* SourceAnimInstance = CurrentMeshComponent->GetAnimInstance();
    if (SourceAnimInstance != nullptr) SourceCurves.CopyFrom(SourceAnimInstance->GetAnimationCurves());
    else SourceCurves.Empty();
}

// The source component is read before the update (PreUpdate), so a pin that changes it takes effect a frame later.
void FAnimNode_CopyPoseFromMesh::Update_AnyThread(const FAnimationUpdateContext& Context) { GetEvaluateGraphExposedInputs().Execute(Context); }

void FAnimNode_CopyPoseFromMesh::Evaluate_AnyThread(FPoseContext& Output) {
    FCompactPose& OutPose = Output.Pose;
    OutPose.ResetToRefPose();
    const USkeletalMesh* CurrentMesh = CurrentlyUsedMesh;
    if (SourceMeshTransformArray.Num() > 0 && CurrentMesh != nullptr) {
        const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
        auto SourceBoneOf = [this, &RequiredBones](FCompactPoseBoneIndex PoseBoneIndex) {
            const int32 MeshBoneIndex = RequiredBones.MakeMeshPoseIndex(PoseBoneIndex).GetInt();
            const int32 SourceBoneIndex = BoneMapToSource.IsValidIndex(MeshBoneIndex) ? BoneMapToSource[MeshBoneIndex] : INDEX_NONE;
            return SourceMeshTransformArray.IsValidIndex(SourceBoneIndex) ? SourceBoneIndex : INDEX_NONE;
        };
        if (bUseMeshPose) {
            FCSPose<FCompactPose> MeshPoses;
            MeshPoses.InitPose(OutPose);
            for (int32 Index = 0; Index < OutPose.GetNumBones(); ++Index) {
                const FCompactPoseBoneIndex PoseBoneIndex(Index);
                const int32 SourceBoneIndex = SourceBoneOf(PoseBoneIndex);
                if (SourceBoneIndex != INDEX_NONE) MeshPoses.SetComponentSpaceTransform(PoseBoneIndex, SourceMeshTransformArray[SourceBoneIndex]);
            }
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(MeshPoses, OutPose);
        } else {
            for (int32 Index = 0; Index < OutPose.GetNumBones(); ++Index) {
                const FCompactPoseBoneIndex PoseBoneIndex(Index);
                const int32 SourceBoneIndex = SourceBoneOf(PoseBoneIndex);
                if (SourceBoneIndex == INDEX_NONE) continue;
                const int32 ParentIndex = CurrentMesh->GetRefSkeleton().GetParentIndex(SourceBoneIndex);
                const FCompactPoseBoneIndex MyParentIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
                // A bone without a parent here takes the source's component space transform.
                if (SourceMeshTransformArray.IsValidIndex(ParentIndex) && MyParentIndex.IsValid())
                    OutPose[PoseBoneIndex] = SourceMeshTransformArray[SourceBoneIndex].GetRelativeTransform(SourceMeshTransformArray[ParentIndex]);
                else
                    OutPose[PoseBoneIndex] = SourceMeshTransformArray[SourceBoneIndex];
            }
        }
    }
    if (bCopyCurves) Output.Curve.CopyFrom(SourceCurves);
}

void FAnimNode_CopyPoseFromMesh::ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, USkeletalMeshComponent* TargetMeshComponent) {
    CurrentlyUsedSourceMeshComponent = nullptr;
    CurrentlyUsedSourceMesh = nullptr;
    CurrentlyUsedTargetMesh = nullptr;
    BoneMapToSource.Reset();
    if (TargetMeshComponent == nullptr || NewSourceMeshComponent == nullptr) return;
    const USkeletalMesh* SourceSkelMesh = NewSourceMeshComponent->GetSkeletalMeshAsset();
    const USkeletalMesh* TargetSkelMesh = TargetMeshComponent->GetSkeletalMeshAsset();
    if (SourceSkelMesh == nullptr || TargetSkelMesh == nullptr) return;
    CurrentlyUsedSourceMeshComponent = NewSourceMeshComponent;
    CurrentlyUsedSourceMesh = SourceSkelMesh;
    CurrentlyUsedTargetMesh = TargetSkelMesh;
    const FReferenceSkeleton& TargetRefSkeleton = TargetSkelMesh->GetRefSkeleton();
    const int32 NumTargetBones = TargetRefSkeleton.GetNum();
    BoneMapToSource.SetNum(NumTargetBones);
    if (SourceSkelMesh == TargetSkelMesh) {
        for (int32 BoneIndex = 0; BoneIndex < NumTargetBones; ++BoneIndex) BoneMapToSource[BoneIndex] = BoneIndex;
        return;
    }
    const int32 SplitBoneIndex = RootBoneToCopy.isNone() ? INDEX_NONE : TargetRefSkeleton.FindBoneIndex(RootBoneToCopy);
    for (int32 BoneIndex = 0; BoneIndex < NumTargetBones; ++BoneIndex) {
        const bool bCopied =
            SplitBoneIndex == INDEX_NONE || BoneIndex == SplitBoneIndex || TargetRefSkeleton.BoneIsChildOf(BoneIndex, SplitBoneIndex);
        BoneMapToSource[BoneIndex] = bCopied ? SourceSkelMesh->GetRefSkeleton().FindBoneIndex(TargetRefSkeleton.GetBoneName(BoneIndex)) : INDEX_NONE;
    }
}

}
