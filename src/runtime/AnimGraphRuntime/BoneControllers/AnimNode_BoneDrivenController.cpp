#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_BoneDrivenController.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Curves/CurveFloat.h"

namespace ue5parse::runtime {

double FAnimNode_BoneDrivenController::ExtractSourceValue(FCSPose<FCompactPose>& Pose) const {
    const FBoneContainer& BoneContainer = Pose.GetPose().GetBoneContainer();
    const FTransform& RefPoseTransform = BoneContainer.GetRefPoseArray()[SourceBone.BoneIndex];
    const FTransform CurrentTransform = Pose.GetLocalSpaceTransform(SourceBone.GetCompactPoseIndex(BoneContainer));
    double SourceValue;
    if (SourceComponent < EComponentType::RotationX) {
        SourceValue = (CurrentTransform.GetLocation() - RefPoseTransform.GetLocation())[SourceComponent - EComponentType::TranslationX];
    } else if (SourceComponent < EComponentType::Scale) {
        const FVector RotationDiff = (CurrentTransform.GetRotation() * RefPoseTransform.GetRotation().Inverse()).Euler();
        SourceValue = RotationDiff[SourceComponent - EComponentType::RotationX];
    } else if (SourceComponent == EComponentType::Scale) {
        SourceValue = CurrentTransform.GetScale3D().GetMax() - RefPoseTransform.GetScale3D().GetMax();
    } else {
        SourceValue = (CurrentTransform.GetScale3D() - RefPoseTransform.GetScale3D())[SourceComponent - EComponentType::ScaleX];
    }
    if (DrivingCurve != nullptr) return DrivingCurve->GetFloatValue(static_cast<float>(SourceValue));
    if (bUseRange) SourceValue = FMath::Lerp(RemappedMin, RemappedMax, FMath::Clamp(FMath::GetRangePct(RangeMin, RangeMax, SourceValue), 0.0, 1.0));
    return SourceValue * Multiplier;
}

void FAnimNode_BoneDrivenController::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) {
    if (SourceComponent == EComponentType::None || DestinationMode == EDrivenDestinationMode::Bone) return;
    // A morph target or material parameter is driven through the curve of the same name.
    Context.Curve.Set(ParameterName, static_cast<float>(ExtractSourceValue(Context.Pose)));
}

void FAnimNode_BoneDrivenController::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    if (SourceComponent == EComponentType::None || DestinationMode != EDrivenDestinationMode::Bone) return;
    const double FinalDriverValue = ExtractSourceValue(Output.Pose);
    const FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
    const FTransform OriginalLocalTM = Output.Pose.GetLocalSpaceTransform(TargetBoneIndex);
    FVector NewTrans = OriginalLocalTM.GetTranslation();
    FVector NewScale = OriginalLocalTM.GetScale3D();
    FQuat NewRot = OriginalLocalTM.GetRotation();
    const bool bAffectRotation = bAffectTargetRotationX || bAffectTargetRotationY || bAffectTargetRotationZ;
    switch (ModificationMode) {
        case EDrivenBoneModificationMode::AddToInput: {
            if (bAffectTargetTranslationX) NewTrans.X += FinalDriverValue;
            if (bAffectTargetTranslationY) NewTrans.Y += FinalDriverValue;
            if (bAffectTargetTranslationZ) NewTrans.Z += FinalDriverValue;
            if (bAffectRotation) {
                const FVector NewRotDeltaEuler(bAffectTargetRotationX ? FinalDriverValue : 0.0, bAffectTargetRotationY ? FinalDriverValue : 0.0,
                                               bAffectTargetRotationZ ? FinalDriverValue : 0.0);
                NewRot = NewRot * FQuat::MakeFromEuler(NewRotDeltaEuler);
            }
            if (bAffectTargetScaleX) NewScale.X += FinalDriverValue;
            if (bAffectTargetScaleY) NewScale.Y += FinalDriverValue;
            if (bAffectTargetScaleZ) NewScale.Z += FinalDriverValue;
            break;
        }
        case EDrivenBoneModificationMode::ReplaceComponent: {
            if (bAffectTargetTranslationX) NewTrans.X = FinalDriverValue;
            if (bAffectTargetTranslationY) NewTrans.Y = FinalDriverValue;
            if (bAffectTargetTranslationZ) NewTrans.Z = FinalDriverValue;
            if (bAffectRotation) {
                FVector NewRotEuler = NewRot.Euler();
                if (bAffectTargetRotationX) NewRotEuler.X = FinalDriverValue;
                if (bAffectTargetRotationY) NewRotEuler.Y = FinalDriverValue;
                if (bAffectTargetRotationZ) NewRotEuler.Z = FinalDriverValue;
                NewRot = FQuat::MakeFromEuler(NewRotEuler);
            }
            if (bAffectTargetScaleX) NewScale.X = FinalDriverValue;
            if (bAffectTargetScaleY) NewScale.Y = FinalDriverValue;
            if (bAffectTargetScaleZ) NewScale.Z = FinalDriverValue;
            break;
        }
        case EDrivenBoneModificationMode::AddToRefPose: {
            const FTransform& RefPoseTransform = Output.Pose.GetPose().GetRefPose(TargetBoneIndex);
            if (bAffectTargetTranslationX) NewTrans.X = RefPoseTransform.GetTranslation().X + FinalDriverValue;
            if (bAffectTargetTranslationY) NewTrans.Y = RefPoseTransform.GetTranslation().Y + FinalDriverValue;
            if (bAffectTargetTranslationZ) NewTrans.Z = RefPoseTransform.GetTranslation().Z + FinalDriverValue;
            if (bAffectRotation) {
                // The driven components start from the reference pose; the value is a delta on top.
                const FVector RefPoseRotEuler = RefPoseTransform.GetRotation().Euler();
                FVector SourceRotationEuler = NewRot.Euler();
                FVector NewRotDeltaEuler = FVector::ZeroVector;
                if (bAffectTargetRotationX) {
                    SourceRotationEuler.X = RefPoseRotEuler.X;
                    NewRotDeltaEuler.X = FinalDriverValue;
                }
                if (bAffectTargetRotationY) {
                    SourceRotationEuler.Y = RefPoseRotEuler.Y;
                    NewRotDeltaEuler.Y = FinalDriverValue;
                }
                if (bAffectTargetRotationZ) {
                    SourceRotationEuler.Z = RefPoseRotEuler.Z;
                    NewRotDeltaEuler.Z = FinalDriverValue;
                }
                NewRot = FQuat::MakeFromEuler(SourceRotationEuler) * FQuat::MakeFromEuler(NewRotDeltaEuler);
            }
            if (bAffectTargetScaleX) NewScale.X = RefPoseTransform.GetScale3D().X + FinalDriverValue;
            if (bAffectTargetScaleY) NewScale.Y = RefPoseTransform.GetScale3D().Y + FinalDriverValue;
            if (bAffectTargetScaleZ) NewScale.Z = RefPoseTransform.GetScale3D().Z + FinalDriverValue;
            break;
        }
    }
    FTransform ModifiedTM(NewRot, NewTrans, NewScale);
    const FCompactPoseBoneIndex ParentIndex = Output.Pose.GetPose().GetParentBoneIndex(TargetBoneIndex);
    if (ParentIndex != INDEX_NONE) ModifiedTM *= Output.Pose.GetComponentSpaceTransform(ParentIndex);
    OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, ModifiedTM));
}

bool FAnimNode_BoneDrivenController::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return SourceBone.IsValidToEvaluate(RequiredBones) && (TargetBone.IsValidToEvaluate(RequiredBones) || DestinationMode != EDrivenDestinationMode::Bone);
}

void FAnimNode_BoneDrivenController::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    SourceBone.Initialize(RequiredBones);
    TargetBone.Initialize(RequiredBones);
}

}
