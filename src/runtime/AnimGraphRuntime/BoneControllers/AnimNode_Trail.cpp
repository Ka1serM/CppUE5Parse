#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_Trail.h"

#include "ue5parse/runtime/AnimationCore/AngularLimit.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

FAnimNode_Trail::FAnimNode_Trail() {
    FRichCurve* TrailRelaxRichCurve = TrailRelaxationSpeed.GetRichCurve();
    TrailRelaxRichCurve->AddKey(0.f, 10.f);
    TrailRelaxRichCurve->AddKey(1.f, 5.f);
}

void FAnimNode_Trail::UpdateInternal(const FAnimationUpdateContext& Context) {
    FAnimNode_SkeletalControlBase::UpdateInternal(Context);
    ThisTimstep += Context.GetDeltaTime();
}

void FAnimNode_Trail::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const float TimeStep = MaxDeltaTime > 0.f ? FMath::Clamp(ThisTimstep, 0.f, MaxDeltaTime) : ThisTimstep;
    ThisTimstep = 0.f;
    if (ChainBoneIndices.Num() <= 0) return;

    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

    // The base of the simulation in world space: the base joint, or the component.
    FTransform BaseTransform;
    if (BaseJoint.IsValidToEvaluate(BoneContainer)) {
        const FCompactPoseBoneIndex BasePoseIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BaseJoint.BoneIndex));
        const FTransform BaseBoneTransform = Output.Pose.GetComponentSpaceTransform(BasePoseIndex);
        BaseTransform = BaseBoneTransform * ComponentTransform;
    } else {
        BaseTransform = ComponentTransform;
    }

    OutBoneTransforms.AddZeroed(ChainLength);

    // When the control gets weight again, the chain starts from the animated pose.
    const bool bHasValidStrength = Alpha > 0.f;
    if (bHasValidStrength && !bHadValidStrength) {
        for (int32 i = 0; i < ChainBoneIndices.Num(); i++) {
            if (BoneContainer.Contains(static_cast<FBoneIndexType>(ChainBoneIndices[i]))) {
                const FCompactPoseBoneIndex ChildIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i]));
                TrailBoneLocations[i] = Output.Pose.GetComponentSpaceTransform(ChildIndex).GetTranslation();
            } else {
                TrailBoneLocations[i] = FVector::ZeroVector;
            }
        }
        OldBaseTransform = BaseTransform;
    }
    bHadValidStrength = bHasValidStrength;

    // The movement of the base since the last frame.
    const FTransform OldToNewTM = OldBaseTransform.GetRelativeTransform(BaseTransform);

    if (!FakeVelocity.IsZero()) {
        FVector FakeMovement = -FakeVelocity * TimeStep;
        if (bActorSpaceFakeVel) {
            FTransform BoneToWorld(Output.AnimInstanceProxy->GetActorTransform());
            BoneToWorld.RemoveScaling();
            FakeMovement = BoneToWorld.TransformVector(FakeMovement);
        }
        FakeMovement = BaseTransform.InverseTransformVector(FakeMovement);
        for (int32 i = 1; i < TrailBoneLocations.Num(); i++) TrailBoneLocations[i] += FakeMovement;
    }

    // The root of the chain keeps its animated transform.
    const FCompactPoseBoneIndex RootIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[0]));
    const FTransform& ChainTransform = Output.Pose.GetComponentSpaceTransform(RootIndex);
    OutBoneTransforms[0] = FBoneTransform(RootIndex, ChainTransform);
    TrailBoneLocations[0] = ChainTransform.GetTranslation();

    for (int32 i = 1; i < ChainBoneIndices.Num(); i++) {
        const FCompactPoseBoneIndex ParentIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i - 1]));
        const FVector ParentPos = TrailBoneLocations[i - 1];
        const FVector ParentAnimPos = Output.Pose.GetComponentSpaceTransform(ParentIndex).GetTranslation();
        const FCompactPoseBoneIndex ChildIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i]));
        // Where the bone was last frame, moved by the base.
        const FVector ChildPos = OldToNewTM.TransformPosition(TrailBoneLocations[i]);
        const FVector ChildAnimPos = Output.Pose.GetComponentSpaceTransform(ChildIndex).GetTranslation();
        // Where the animated pose would put it relative to its parent.
        const FVector TargetDelta = ChildAnimPos - ParentAnimPos;
        const FVector ChildTarget = ParentPos + TargetDelta;
        const FVector Error = ChildTarget - ChildPos;
        const float SpeedScale = RelaxationSpeedScaleInputProcessor.ApplyTo(RelaxationSpeedScale, TimeStep);
        const float Correction = FMath::Clamp<float>(TimeStep * SpeedScale * PerJointTrailData[i].TrailRelaxationSpeedPerSecond, 0.f, 1.f);
        TrailBoneLocations[i] = ChildPos + Error * Correction;

        if (bLimitStretch) {
            const double RefPoseLength = TargetDelta.Size();
            const FVector CurrentDelta = TrailBoneLocations[i] - TrailBoneLocations[i - 1];
            const double CurrentLength = CurrentDelta.Size();
            if (CurrentLength - RefPoseLength > StretchLimit && CurrentLength > UE_DOUBLE_SMALL_NUMBER) {
                const FVector CurrentDir = CurrentDelta / CurrentLength;
                TrailBoneLocations[i] = TrailBoneLocations[i - 1] + CurrentDir * (RefPoseLength + StretchLimit);
            }
        }

        if (bUsePlanarLimit) {
            for (const FAnimPhysPlanarLimit& PlanarLimit : PlanarLimits) {
                FTransform LimitPlaneTransform = PlanarLimit.PlaneTransform;
                if (PlanarLimit.DrivingBone.IsValidToEvaluate(BoneContainer)) {
                    const FCompactPoseBoneIndex DrivingBoneIndex = PlanarLimit.DrivingBone.GetCompactPoseIndex(BoneContainer);
                    LimitPlaneTransform *= Output.Pose.GetComponentSpaceTransform(DrivingBoneIndex);
                }
                // UE's FPlane(Base, Normal) and PlaneDot.
                const FVector PlaneNormal = LimitPlaneTransform.GetUnitAxis(2);
                const double PlaneW = LimitPlaneTransform.GetLocation() | PlaneNormal;
                const double DistanceFromPlane = (TrailBoneLocations[i] | PlaneNormal) - PlaneW;
                if (DistanceFromPlane < 0.0) TrailBoneLocations[i] -= DistanceFromPlane * PlaneNormal;
            }
        }

        OutBoneTransforms[i] = FBoneTransform(ChildIndex, Output.Pose.GetComponentSpaceTransform(ChildIndex));
        OutBoneTransforms[i].Transform.SetTranslation(TrailBoneLocations[i]);

        // The parent turns toward its child's new position.
        if (bReorientParentToChild) {
            FVector CurrentBoneDir = OutBoneTransforms[i - 1].Transform.TransformVector(GetAlignVector(ChainBoneAxis, bInvertChainBoneAxis));
            CurrentBoneDir = CurrentBoneDir.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER);
            const FVector DeltaTranslation = OutBoneTransforms[i].Transform.GetTranslation() - OutBoneTransforms[i - 1].Transform.GetTranslation();
            const FVector NewBoneDir = FVector(DeltaTranslation).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER);
            const FQuat DeltaLookQuat = FQuat::FindBetweenNormals(CurrentBoneDir, NewBoneDir);
            const FQuat ParentRotation = OutBoneTransforms[i - 1].Transform.GetRotation();
            FQuat NewRotation = DeltaLookQuat * ParentRotation;

            if (bLimitRotation) {
                FQuat GrandParentRotation = FQuat::Identity;
                if (i == 1) {
                    const FCompactPoseBoneIndex GrandParentIndex = BoneContainer.GetParentBoneIndex(ParentIndex);
                    if (GrandParentIndex != INDEX_NONE) GrandParentRotation = Output.Pose.GetComponentSpaceTransform(GrandParentIndex).GetRotation();
                } else {
                    GrandParentRotation = OutBoneTransforms[i - 2].Transform.GetRotation();
                }
                FQuat NewLocalRotation = GrandParentRotation.Inverse() * NewRotation;
                const FQuat& RefRotation = BoneContainer.GetRefPoseTransform(ParentIndex).GetRotation();
                const FRotationLimit& RotationLimit = RotationLimits[i - 1];
                if (AnimationCore::ConstrainAngularRangeUsingEuler(NewLocalRotation, RefRotation, RotationLimit.LimitMin + RotationOffsets[i - 1],
                                                                   RotationLimit.LimitMax + RotationOffsets[i - 1])) {
                    // The child moves with the limited rotation.
                    NewRotation = GrandParentRotation * NewLocalRotation;
                    const FVector NewTranslation = NewRotation.Vector() * DeltaTranslation.Size();
                    const FVector AdjustedLocation =
                        FMath::Lerp(DeltaTranslation, NewTranslation, static_cast<double>(Correction)) + OutBoneTransforms[i - 1].Transform.GetTranslation();
                    OutBoneTransforms[i].Transform.SetTranslation(AdjustedLocation);
                    TrailBoneLocations[i] = AdjustedLocation;
                }
            }
            OutBoneTransforms[i - 1].Transform.SetRotation(NewRotation);
        }
    }

    FQuat LeafRotation = FQuat::FastLerp(OutBoneTransforms[ChainLength - 2].Transform.GetRotation(),
                                         OutBoneTransforms[ChainLength - 1].Transform.GetRotation(), LastBoneRotationAnimAlphaBlend);
    LeafRotation.Normalize();
    OutBoneTransforms[ChainLength - 1].Transform.SetRotation(LeafRotation);

    OldBaseTransform = BaseTransform;
}

bool FAnimNode_Trail::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    if (TrailBone.IsValidToEvaluate(RequiredBones)) {
        for (int32 ChainIndex : ChainBoneIndices) {
            if (ChainIndex == INDEX_NONE) return false;
            if (!RequiredBones.Contains(static_cast<FBoneIndexType>(ChainIndex))) return false;
        }
    }
    return ChainBoneIndices.Num() > 0;
}

void FAnimNode_Trail::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    TrailBone.Initialize(RequiredBones);
    BaseJoint.Initialize(RequiredBones);
    ChainBoneIndices.Reset();
    if (ChainLength > 1 && TrailBone.IsValidToEvaluate(RequiredBones)) {
        // The chain from the base down to the trail bone, walked up from the trail bone.
        ChainBoneIndices.AddZeroed(ChainLength);
        int32 WalkBoneIndex = TrailBone.BoneIndex;
        ChainBoneIndices[ChainLength - 1] = WalkBoneIndex;
        for (int32 i = 1; i < ChainLength; i++) {
            const int32 TransformIndex = ChainLength - (i + 1);
            if (WalkBoneIndex == INDEX_NONE || WalkBoneIndex == 0) {
                ChainBoneIndices[TransformIndex] = INDEX_NONE;
            } else {
                WalkBoneIndex = RequiredBones.GetParentBoneIndex(WalkBoneIndex);
                ChainBoneIndices[TransformIndex] = WalkBoneIndex;
            }
        }
    }
    for (FAnimPhysPlanarLimit& PlanarLimit : PlanarLimits) PlanarLimit.DrivingBone.Initialize(RequiredBones);
}

FVector FAnimNode_Trail::GetAlignVector(EAxis::Type AxisOption, bool bInvert) {
    FVector AxisDir = AxisOption == EAxis::X ? FVector(1, 0, 0) : (AxisOption == EAxis::Y ? FVector(0, 1, 0) : FVector(0, 0, 1));
    if (bInvert) AxisDir *= -1.f;
    return AxisDir;
}

void FAnimNode_Trail::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
    PerJointTrailData.Reset();
    TrailBoneLocations.Reset();
    if (ChainLength > 1) {
        PerJointTrailData.AddZeroed(ChainLength);
        TrailBoneLocations.AddZeroed(ChainLength);
        const float Interval = ChainLength > 1 ? 1.f / (ChainLength - 1) : 0.f;
        const FRichCurve* TrailRelaxRichCurve = TrailRelaxationSpeed.GetRichCurveConst();
        for (int32 Idx = 0; Idx < ChainLength; ++Idx) PerJointTrailData[Idx].TrailRelaxationSpeedPerSecond = TrailRelaxRichCurve->Eval(Interval * Idx);
    }
    RelaxationSpeedScaleInputProcessor.Reinitialize();
}

}
