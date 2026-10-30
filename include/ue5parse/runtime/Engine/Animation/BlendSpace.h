#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/AnimInterpFilter.h"

#include "BlendSpace.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;
class UBlendProfile;
struct FAnimationPoseData;
struct FBoneContainer;
struct FCompactPose;
struct FPoseLink;
class FAnimInstanceProxy;

// FBlendSample: an animation at a point of the blend space.
USTRUCT()
struct FBlendSample {
    GENERATED_BODY()
    UPROPERTY()
    UAnimSequence* Animation = nullptr;
    UPROPERTY()
    FVector SampleValue = FVector::ZeroVector;
    UPROPERTY()
    float RateScale = 1;
};

// FEditorElement: the samples of a grid point and their weights; unused indices are INDEX_NONE.
USTRUCT()
struct FEditorElement {
    GENERATED_BODY()
    static constexpr int32 MAX_VERTICES = 3;
    UPROPERTY()
    int32 Indices[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    float Weights[3] = {0, 0, 0};
};

// A grid point of the input and its weight there.
struct FGridBlendSample {
    FEditorElement GridElement;
    float BlendWeight = 0;
};

// A segment of a 1D blend space's samples on the normalized axis.
USTRUCT()
struct FBlendSpaceSegment {
    GENERATED_BODY()
    static constexpr int32 NUM_VERTICES = 2;
    UPROPERTY()
    int32 SampleIndices[2] = {INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    float Vertices[2] = {0.f, 0.f};
};

// An edge of a triangle: its outward normal, the triangle across it, and along the perimeter the triangles and
// vertices next to it.
USTRUCT()
struct FBlendSpaceTriangleEdgeInfo {
    GENERATED_BODY()
    UPROPERTY()
    FVector2D Normal = FVector2D(0.f, 0.f);
    UPROPERTY()
    int32 NeighbourTriangleIndex = INDEX_NONE;
    UPROPERTY()
    int32 AdjacentPerimeterTriangleIndices[2] = {INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    int32 AdjacentPerimeterVertexIndices[2] = {INDEX_NONE, INDEX_NONE};
};

USTRUCT()
struct FBlendSpaceTriangle {
    GENERATED_BODY()
    static constexpr int32 NUM_VERTICES = 3;
    UPROPERTY()
    int32 SampleIndices[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    FVector2D Vertices[3] = {FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(0.f, 0.f)};
    UPROPERTY()
    FBlendSpaceTriangleEdgeInfo EdgeInfo[3];
};

USTRUCT()
struct FWeightedBlendSample {
    GENERATED_BODY()
    UPROPERTY()
    int32 SampleIndex = INDEX_NONE;
    UPROPERTY()
    float SampleWeight = 0;

    FWeightedBlendSample() = default;
    FWeightedBlendSample(int32 Index, float Weight) : SampleIndex(Index), SampleWeight(Weight) {}
};

// The cooked triangulation of the samples (segments in 1D), on the normalized axes. A search walks from the last
// triangle to the one that holds the input; an input outside projects onto the perimeter.
USTRUCT()
struct FBlendSpaceData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FBlendSpaceSegment> Segments;
    UPROPERTY()
    TArray<FBlendSpaceTriangle> Triangles;

    void GetSamples(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices, const FVector& InSamplePosition,
                    int32& InOutTriangulationIndex) const;
    bool IsEmpty() const { return Segments.Num() == 0 && Triangles.Num() == 0; }

private:
    void GetSamples1D(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices, const FVector& InSamplePosition,
                      int32& InOutSegmentIndex) const;
    void GetSamples2D(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices, const FVector& InSamplePosition,
                      int32& InOutTriangleIndex) const;
};

USTRUCT()
struct FBlendParameter {
    GENERATED_BODY()
    UPROPERTY()
    FString DisplayName = FString("None");
    UPROPERTY()
    float Min = 0;
    UPROPERTY()
    float Max = 100;
    UPROPERTY()
    int32 GridNum = 4;
    UPROPERTY()
    bool bSnapToGrid = false;
    UPROPERTY()
    bool bWrapInput = false;

    float GetRange() const { return Max - Min; }
    float GetGridSize() const { return GetRange() / static_cast<float>(GridNum); }
};

USTRUCT()
struct FInterpolationParameter {
    GENERATED_BODY()
    UPROPERTY()
    float InterpolationTime = 0;
    UPROPERTY()
    float DampingRatio = 1;
    UPROPERTY()
    float MaxSpeed = 0;
    UPROPERTY()
    TEnumAsByte<EFilterInterpolationType> InterpolationType = BSIT_SpringDamper;
};

// A bone (and its children) whose sample weights move to their targets at their own speed.
USTRUCT()
struct FPerBoneInterpolation {
    GENERATED_BODY()
    UPROPERTY()
    FBoneReference BoneReference;
    UPROPERTY()
    float InterpolationSpeedPerSec = 6.f;

    void Initialize(const USkeleton* Skeleton) { BoneReference.Initialize(Skeleton); }
};

USTRUCT()
struct FBlendSpaceBlendProfile {
    GENERATED_BODY()
    UPROPERTY()
    UBlendProfile* BlendProfile = nullptr;
    UPROPERTY()
    float TargetWeightInterpolationSpeedPerSec = 0.0f;
};

enum EBlendSpaceAxis : int32 { BSA_None, BSA_X, BSA_Y };
enum class EPreferredTriangulationDirection : uint8 { None, Tangential, Radial };
enum class EBlendSpacePerBoneBlendMode : uint8 { ManualPerBoneOverride, BlendProfile };

namespace ENotifyTriggerMode {
enum Type : int32 { AllAnimations, HighestWeightedAnimation, None };
}

// UBlendSpace (UE's Animation/BlendSpace.h): animations placed on one or two axes and blended by position. A player
// gives an input each frame; the blend space filters it, finds the weighted samples (by the cooked triangulation or
// grid), smooths the weights, and advances a normalized time that every sample follows.
UCLASS()
class UBlendSpace : public UAnimationAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bContainsRotationOffsetMeshSpaceSamples = false;
    UPROPERTY()
    FInterpolationParameter InterpolationParam[3];
    UPROPERTY()
    float TargetWeightInterpolationSpeedPerSec = 0;
    // UE 5.3's member initializer.
    UPROPERTY()
    bool bTargetWeightInterpolationEaseInOut = true;
    UPROPERTY()
    bool bAllowMeshSpaceBlending = false;
    UPROPERTY()
    bool bLoop = true;
    UPROPERTY()
    float AnimLength = 0;
    UPROPERTY()
    TEnumAsByte<ENotifyTriggerMode::Type> NotifyTriggerMode = ENotifyTriggerMode::HighestWeightedAnimation;
    UPROPERTY()
    bool bInterpolateUsingGrid = false;
    UPROPERTY()
    EPreferredTriangulationDirection PreferredTriangulationDirection = EPreferredTriangulationDirection::Tangential;
    UPROPERTY()
    EBlendSpacePerBoneBlendMode PerBoneBlendMode = EBlendSpacePerBoneBlendMode::ManualPerBoneOverride;
    UPROPERTY()
    TArray<FPerBoneInterpolation> ManualPerBoneOverrides;
    UPROPERTY()
    FBlendSpaceBlendProfile PerBoneBlendProfile;
    UPROPERTY()
    int32 SampleIndexWithMarkers = INDEX_NONE;
    UPROPERTY()
    TArray<FBlendSample> SampleData;
    UPROPERTY()
    TArray<FEditorElement> GridSamples;
    UPROPERTY()
    FBlendSpaceData BlendSpaceData;
    UPROPERTY()
    FBlendParameter BlendParameters[3];
    UPROPERTY()
    TEnumAsByte<EBlendSpaceAxis> AxisToScaleAnimation = BSA_None;
    // UBlendSpace's constructor sets {0, 1}, UBlendSpace1D's {0}.
    UPROPERTY()
    TArray<int32> DimensionIndices;

    explicit UBlendSpace(const FObjectInitializer& ObjectInitializer);
    void PostLoad() override;
    float GetPlayLength() const override { return 1.f; }
    void TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
    bool IsValidAdditive() const;

    const TArray<FBlendSample>& GetBlendSamples() const { return SampleData; }
    const FBlendSample& GetBlendSample(int32 SampleIndex) const { return SampleData[SampleIndex]; }
    const FBlendParameter& GetBlendParameter(int32 Index) const { return BlendParameters[Index]; }
    virtual EBlendSpaceAxis GetAxisToScale() const { return AxisToScaleAnimation; }

    // The blended pose of the samples. With pose links (a blend space graph), sample i is the pose of link i.
    void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, const FAnimExtractContext& ExtractionContext,
                          FAnimationPoseData& OutAnimationPoseData) const;
    void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, std::span<FPoseLink> InPoseLinks, FAnimInstanceProxy* InProxy,
                          bool bInExpectsAdditivePose, const FAnimExtractContext& ExtractionContext, FAnimationPoseData& OutAnimationPoseData) const;
    // The samples at an input, sorted by weight, heaviest first, with weights that sum to 1. bCombineAnimations merges
    // samples of the same animation.
    bool GetSamplesFromBlendInput(const FVector& BlendInput, TArray<FBlendSampleData>& OutSampleDataList, int32& InOutCachedTriangulationIndex,
                                  bool bCombineAnimations) const;
    float GetAnimationLengthFromSampleData(const TArray<FBlendSampleData>& SampleDataList) const;
    void InitializeFilter(FBlendFilter* Filter, int32 NumDimensions = 2) const;
    void UpdateFilterParams(FBlendFilter* Filter) const;
    FVector GetClampedAndWrappedBlendInput(const FVector& BlendInput) const;
    // The samples at the input, their weights smoothed from the previous ones.
    bool UpdateBlendSamples(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutSampleDataCache,
                            int32& InOutCachedTriangulationIndex) const;
    FVector FilterInput(FBlendFilter* Filter, const FVector& BlendInput, float DeltaTime) const;
    // For each compact bone, the index of the per bone smoothing that covers it (the bone's or its nearest listed
    // parent's), or INDEX_NONE.
    void GetPerBoneInterpolationIndices(const FBoneContainer& RequiredBones, TArray<int32>& OutPerBoneIndices) const;
    bool HasPerBoneBlend() const { return PerBoneBlendValues.Num() > 0; }

protected:
    void InitializePerBoneBlend();
    void GetRawSamplesFromBlendInput(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const;
    FVector GetClampedBlendInput(const FVector& BlendInput) const;
    FVector ConvertBlendInputToGridSpace(const FVector& BlendInput) const;
    FVector GetNormalizedBlendInput(const FVector& BlendInput) const;
    const FEditorElement* GetGridSampleInternal(int32 Index) const { return GridSamples.IsValidIndex(Index) ? &GridSamples[Index] : nullptr; }
    bool InterpolateWeightOfSampleData(float DeltaTime, const TArray<FBlendSampleData>& OldSampleDataList,
                                       const TArray<FBlendSampleData>& NewSampleDataList, TArray<FBlendSampleData>& FinalSampleDataList) const;
    bool ContainsMatchingSamples(EAdditiveAnimationType AdditiveType) const;
    void ResetToRefPose(FCompactPose& OutPose) const;

    // The per bone smoothing in use: the manual overrides, or the blend profile's bones.
    TArray<FPerBoneInterpolation> PerBoneBlendValues;

private:
    bool UpdateBlendSamples_Internal(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutOldSampleDataList,
                                     TArray<FBlendSampleData>& InOutSampleDataCache, int32& InOutCachedTriangulationIndex) const;
    void GetRawSamplesFromBlendInput1D(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const;
    void GetRawSamplesFromBlendInput2D(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const;
    const FEditorElement* GetEditorElement(int32 XIndex, int32 YIndex) const {
        return GetGridSampleInternal(YIndex * (BlendParameters[0].GridNum + 1) + XIndex);
    }
};

}
