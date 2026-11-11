#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_ModifyBones.h"

namespace ue5parse::runtime {

void FAnimNode_ModifyBones::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    FAnimNode_ModifyBone Modification;
    Modification.TranslationMode = TranslationMode;
    Modification.RotationMode = RotationMode;
    Modification.ScaleMode = ScaleMode;
    Modification.TranslationSpace = TranslationSpace;
    Modification.RotationSpace = RotationSpace;
    Modification.ScaleSpace = ScaleSpace;
    for (int32 Index = 0; Index < BonesToModify.Num(); ++Index) {
        if (!BonesToModify[Index].IsValidToEvaluate(BoneContainer))
            continue;
        const FCompactPoseBoneIndex BoneIndex = BonesToModify[Index].GetCompactPoseIndex(BoneContainer);
        Modification.Translation = Translations.IsValidIndex(Index) ? Translations[Index] : FVector::ZeroVector;
        Modification.Rotation = Rotations.IsValidIndex(Index) ? Rotations[Index] : FRotator::ZeroRotator;
        Modification.Scale = Scales.IsValidIndex(Index) ? Scales[Index] : FVector(1.0);
        OutBoneTransforms.Add(FBoneTransform(BoneIndex, Modification.ModifyBoneTransform(Output, BoneIndex)));
    }
    OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

bool FAnimNode_ModifyBones::IsValidToEvaluate(const USkeleton*, const FBoneContainer& RequiredBones) {
    for (FBoneReference& Bone : BonesToModify)
        if (Bone.IsValidToEvaluate(RequiredBones))
            return true;
    return false;
}

void FAnimNode_ModifyBones::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    for (FBoneReference& Bone : BonesToModify)
        Bone.Initialize(RequiredBones);
}

}
