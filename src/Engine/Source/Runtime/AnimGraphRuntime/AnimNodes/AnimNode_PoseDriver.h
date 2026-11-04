#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseHandler.h"
#include "Engine/Source/Runtime/AnimGraphRuntime/RBF/RBFSolver.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_PoseDriver.generated.h"

namespace ue5parse::runtime {

enum class EPoseDriverSource : uint8 { Rotation, Translation };

enum class EPoseDriverOutput : uint8 { DrivePoses, DriveCurves };

USTRUCT()
struct FPoseDriverTransform {
    GENERATED_BODY()
    UPROPERTY()
    FVector TargetTranslation = FVector::ZeroVector;
    UPROPERTY()
    FRotator TargetRotation = FRotator::ZeroRotator;
};

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

    int32 PoseCurveIndex = INDEX_NONE;
};

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

    FAnimNode_PoseDriver() { RBFParams.DistanceMethod = ERBFDistanceMethod::SwingAngle; }

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

protected:
    void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;

private:
    bool IsBoneDriven(FName BoneName) const;
    void GetRBFTargets(TArray<FRBFTarget>& OutTargets, const FBoneContainer& BoneContainer) const;

    TArray<FRBFOutputWeight> OutputWeights;
    bool bHasOnlyDriveBones = false;
    TArray<FCompactPoseBoneIndex> BonesToFilter;
    FRBFEntry RBFInput;
    TArray<FRBFTarget> RBFTargets;
};

}
