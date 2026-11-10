#pragma once

#include "Engine/Source/Runtime/Engine/AnimInterpFilter.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "BlendSpace.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;
class UBlendProfile;
struct FAnimationPoseData;
struct FBoneContainer;
struct FCompactPose;
struct FPoseLink;
class FAnimInstanceProxy;

USTRUCT()
struct FBlendSample {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAnimSequence> Animation = nullptr;
    UPROPERTY()
    FVector SampleValue = FVector::ZeroVector;
    UPROPERTY()
    float RateScale = 1;
};

USTRUCT()
struct FEditorElement {
    GENERATED_BODY()
    static constexpr int32 MAX_VERTICES = 3;
    UPROPERTY()
    int32 Indices[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    float Weights[3] = {0, 0, 0};
};

struct FGridBlendSample {
    FEditorElement GridElement;
    float BlendWeight = 0;
};

USTRUCT()
struct FBlendSpaceSegment {
    GENERATED_BODY()
    static constexpr int32 NUM_VERTICES = 2;
    UPROPERTY()
    int32 SampleIndices[2] = {INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    float Vertices[2] = {0.f, 0.f};
};

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
    TObjectPtr<UBlendProfile> BlendProfile = nullptr;
    UPROPERTY()
    float TargetWeightInterpolationSpeedPerSec = 0.0f;
};

enum EBlendSpaceAxis : int32 { BSA_None, BSA_X, BSA_Y };
enum class EPreferredTriangulationDirection : uint8 { None, Tangential, Radial };
enum class EBlendSpacePerBoneBlendMode : uint8 { ManualPerBoneOverride, BlendProfile };

namespace ENotifyTriggerMode {

enum Type : int32 { AllAnimations, HighestWeightedAnimation, None };

}

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
    UPROPERTY()
    TArray<int32> DimensionIndices;

    explicit UBlendSpace(const FObjectInitializer& ObjectInitializer);
    void PostLoad() override;
    float GetPlayLength() const override { return 1.f; }
    void TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
    bool IsValidAdditive() const;

    const FBlendSample& GetBlendSample(int32 SampleIndex) const { return SampleData[SampleIndex]; }
    virtual EBlendSpaceAxis GetAxisToScale() const { return AxisToScaleAnimation; }

    void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, const FAnimExtractContext& ExtractionContext,
                          FAnimationPoseData& OutAnimationPoseData) const;
    void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, std::span<FPoseLink> InPoseLinks, FAnimInstanceProxy* InProxy,
                          bool bInExpectsAdditivePose, const FAnimExtractContext& ExtractionContext, FAnimationPoseData& OutAnimationPoseData) const;
    bool GetSamplesFromBlendInput(const FVector& BlendInput, TArray<FBlendSampleData>& OutSampleDataList, int32& InOutCachedTriangulationIndex,
                                  bool bCombineAnimations) const;
    float GetAnimationLengthFromSampleData(const TArray<FBlendSampleData>& SampleDataList) const;
    void InitializeFilter(FBlendFilter* Filter, int32 NumDimensions = 2) const;
    FVector GetClampedAndWrappedBlendInput(const FVector& BlendInput) const;
    bool UpdateBlendSamples(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutSampleDataCache,
                            int32& InOutCachedTriangulationIndex) const;
    FVector FilterInput(FBlendFilter* Filter, const FVector& BlendInput, float DeltaTime) const;
    void GetPerBoneInterpolationIndices(const FBoneContainer& RequiredBones, TArray<int32>& OutPerBoneIndices) const;

protected:
    void InitializePerBoneBlend();
    void GetRawSamplesFromBlendInput(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const;
    FVector GetClampedBlendInput(const FVector& BlendInput) const;
    FVector ConvertBlendInputToGridSpace(const FVector& BlendInput) const;
    FVector GetNormalizedBlendInput(const FVector& BlendInput) const;
    const FEditorElement* GetGridSampleInternal(int32 Index) const { return GridSamples.IsValidIndex(Index) ? &GridSamples[Index] : nullptr; }
    bool InterpolateWeightOfSampleData(float DeltaTime, const TArray<FBlendSampleData>& OldSampleDataList, const TArray<FBlendSampleData>& NewSampleDataList,
                                       TArray<FBlendSampleData>& FinalSampleDataList) const;
    bool ContainsMatchingSamples(EAdditiveAnimationType AdditiveType) const;
    void ResetToRefPose(FCompactPose& OutPose) const;

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
