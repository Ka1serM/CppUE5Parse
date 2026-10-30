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
    UPROPERTY()
    TArray<int32> TrackBoneIndices;
    UPROPERTY()
    TArray<FPoseAssetInfluences> TrackPoseInfluenceIndices;
    UPROPERTY()
    TArray<FPoseData> Poses;
    UPROPERTY()
    TArray<FAnimCurveBase> Curves;
};

// UPoseAsset: named poses that blend by weight.
UCLASS()
class UPoseAsset : public UAnimationAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseDataContainer PoseContainer;
    UPROPERTY()
    bool bAdditivePose = false;
    UPROPERTY()
    int32 BasePoseIndex = 0;
    UPROPERTY()
    FName RetargetSource;
    UPROPERTY()
    TArray<FTransform> RetargetSourceAssetReferencePose;

    int32 GetNumPoses() const { return PoseContainer.PoseFNames.Num(); }
    bool IsValidAdditive() const { return bAdditivePose; }
    FName GetRetargetTransformsSourceName() const { return RetargetSource; }
    const TArray<FTransform>& GetRetargetTransforms() const { return RetargetSourceAssetReferencePose; }
};

}
