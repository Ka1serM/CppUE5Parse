#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"

#include "PoseAsset.generated.h"

namespace ue5parse::runtime {

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

USTRUCT()
struct FPoseData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTransform> LocalSpacePose;
    UPROPERTY()
    TArray<float> CurveData;
};

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

    void BlendPoseCurve(const FPoseData& PoseData, FBlendedCurve& InOutCurve, float Weight) const;
};

struct FAnimationPoseData;

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

    const TArray<FName>& GetPoseFNames() const { return PoseContainer.PoseFNames; }
    const TArray<FName>& GetTrackNames() const { return PoseContainer.Tracks; }
    int32 GetPoseIndexByName(FName InPoseName) const { return PoseContainer.PoseFNames.Find(InPoseName); }
    bool GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    bool IsValidAdditive() const { return bAdditivePose; }
    FName GetRetargetTransformsSourceName() const;
    const TArray<FTransform>& GetRetargetTransforms() const;
};

}
