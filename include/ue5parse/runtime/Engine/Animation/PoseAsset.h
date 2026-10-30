#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

#include "PoseAsset.generated.h"

namespace ue5parse::runtime {

// FPoseAssetInfluence: a pose that moves a track, and its transform in that pose.
USTRUCT()
struct FPoseAssetInfluence {
    GENERATED_BODY()
    UPROPERTY()
    int32 PoseIndex = INDEX_NONE;
    UPROPERTY()
    int32 BoneTransformIndex = INDEX_NONE;
};

USTRUCT()
struct FPoseAssetInfluences {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPoseAssetInfluence> Influences;
};

// FPoseData: one pose: the transforms of the tracks it moves and its curve values.
USTRUCT()
struct FPoseData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTransform> LocalSpacePose;
    UPROPERTY()
    TArray<float> CurveData;
};

// FPoseDataContainer (UE's Animation/PoseAsset.h).
USTRUCT()
struct FPoseDataContainer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FName> PoseFNames;
    UPROPERTY()
    TArray<FName> Tracks;
    // The skeleton bone of each track. Transient in UE: UPoseAsset::PostLoad fills it.
    UPROPERTY()
    TArray<int32> TrackBoneIndices;
    UPROPERTY()
    TArray<FPoseAssetInfluences> TrackPoseInfluenceIndices;
    UPROPERTY()
    TArray<FPoseData> Poses;
    UPROPERTY()
    TArray<FAnimCurveBase> Curves;

    // Adds the curve values of a pose times the weight to the curve.
    void BlendPoseCurve(const FPoseData& PoseData, FBlendedCurve& InOutCurve, float Weight) const;
};

struct FAnimationPoseData;

// UPoseAsset: named poses that blend by weight (UE's Animation/PoseAsset.h). An additive asset holds each pose as the
// difference from its base pose.
UCLASS()
class UPoseAsset : public UAnimationAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseDataContainer PoseContainer;
    UPROPERTY()
    bool bAdditivePose = false;
    UPROPERTY()
    int32 BasePoseIndex = INDEX_NONE;
    UPROPERTY()
    FName RetargetSource;
    UPROPERTY()
    TArray<FTransform> RetargetSourceAssetReferencePose;

    void PostLoad() override;

    int32 GetNumPoses() const { return PoseContainer.PoseFNames.Num(); }
    const TArray<FName>& GetPoseFNames() const { return PoseContainer.PoseFNames; }
    const TArray<FName>& GetTrackNames() const { return PoseContainer.Tracks; }
    int32 GetPoseIndexByName(FName InPoseName) const { return PoseContainer.PoseFNames.Find(InPoseName); }
    // The poses of the context's pose curves blended by their weights. A full pose asset starts from the reference pose
    // and normalizes weights that sum above 1; an additive one starts from the additive identity. False when no pose
    // has weight.
    bool GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    bool IsValidAdditive() const { return bAdditivePose; }
    // The pose the poses were authored on: the asset's own copy, else the skeleton's retarget source of the name.
    FName GetRetargetTransformsSourceName() const;
    const TArray<FTransform>& GetRetargetTransforms() const;
};

}
