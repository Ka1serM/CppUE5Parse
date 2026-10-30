#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseHandler.h"
#include "ue5parse/runtime/AnimGraphRuntime/RBF/RBFSolver.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_PoseDriver.generated.h"

namespace ue5parse::runtime {

enum class EPoseDriverSource : uint8 { Rotation, Translation };

enum class EPoseDriverOutput : uint8 { DrivePoses, DriveCurves };

// FPoseDriverTransform: the transform of one source bone at a target.
USTRUCT()
struct FPoseDriverTransform {
    GENERATED_BODY()
    UPROPERTY()
    FVector TargetTranslation = FVector::ZeroVector;
    UPROPERTY()
    FRotator TargetRotation = FRotator::ZeroRotator;
};

// FPoseDriverTarget: a target of the pose driver and the pose or curve that it drives.
USTRUCT()
struct FPoseDriverTarget {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPoseDriverTransform> BoneTransforms;
    UPROPERTY()
    FRotator TargetRotation = FRotator::ZeroRotator;
    UPROPERTY()
    float TargetScale = 1.f;
    UPROPERTY()
    ERBFDistanceMethod DistanceMethod = ERBFDistanceMethod::DefaultMethod;
    UPROPERTY()
    ERBFFunctionType FunctionType = ERBFFunctionType::DefaultFunction;
    UPROPERTY()
    bool bApplyCustomCurve = false;
    UPROPERTY()
    FRichCurve CustomCurve;
    UPROPERTY()
    FName DrivenName;
    UPROPERTY()
    bool bIsHidden = false;

    // The index of the driven pose's curve in the extract context, or INDEX_NONE.
    int32 PoseCurveIndex = INDEX_NONE;
};

// Weights targets by the distance of the source bones' rotations or translations to them (a radial basis function),
// and drives the poses of a pose asset or curves with the weights (UE's AnimNodes/AnimNode_PoseDriver.h). Corrective
// poses follow a joint with it.
USTRUCT()
struct FAnimNode_PoseDriver : public FAnimNode_PoseHandler {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink SourcePose;
    UPROPERTY()
    TArray<FBoneReference> SourceBones;
    UPROPERTY()
    FBoneReference EvalSpaceBone;
    UPROPERTY()
    bool bEvalFromRefPose = false;
    UPROPERTY()
    TArray<FBoneReference> OnlyDriveBones;
    UPROPERTY()
    TArray<FPoseDriverTarget> PoseTargets;
    UPROPERTY()
    FRBFParams RBFParams;
    UPROPERTY()
    EPoseDriverSource DriveSource = EPoseDriverSource::Rotation;
    UPROPERTY()
    EPoseDriverOutput DriveOutput = EPoseDriverOutput::DrivePoses;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;

    // The pose driver changes the default distance method of its RBF params.
    FAnimNode_PoseDriver() { RBFParams.DistanceMethod = ERBFDistanceMethod::SwingAngle; }

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

protected:
    // A pose curve for each target that drives a pose of the asset.
    void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;

private:
    bool IsBoneDriven(FName BoneName) const;
    // The targets as RBF entries of the drive source; with bEvalFromRefPose, relative to the source bones' reference pose.
    void GetRBFTargets(TArray<FRBFTarget>& OutTargets, const FBoneContainer& BoneContainer) const;

    TArray<FRBFOutputWeight> OutputWeights;
    bool bHasOnlyDriveBones = false;
    // The bones with a track in the asset that are not in OnlyDriveBones: an additive asset leaves them unchanged.
    TArray<FCompactPoseBoneIndex> BonesToFilter;
    FRBFEntry RBFInput;
    TArray<FRBFTarget> RBFTargets;
};

}
