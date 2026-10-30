#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

#include "BlendSpace.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;

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
    UPROPERTY()
    int32 Indices[3] = {INDEX_NONE, INDEX_NONE, INDEX_NONE};
    UPROPERTY()
    float Weights[3] = {0, 0, 0};
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

enum EFilterInterpolationType : int32 {
    BSIT_Average,
    BSIT_Linear,
    BSIT_Cubic,
    BSIT_EaseInOut,
    BSIT_ExponentialDecay,
    BSIT_SpringDamper,
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

enum EBlendSpaceAxis : int32 { BSA_None, BSA_X, BSA_Y };

// UBlendSpace (UE's Animation/BlendSpace.h): animations placed on one or two axes and blended by position.
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
    bool bInterpolateUsingGrid = false;
    UPROPERTY()
    TArray<FBlendSample> SampleData;
    UPROPERTY()
    TArray<FEditorElement> GridSamples;
    UPROPERTY()
    FBlendParameter BlendParameters[3];
    UPROPERTY()
    TEnumAsByte<EBlendSpaceAxis> AxisToScaleAnimation = BSA_None;
    // UBlendSpace's constructor sets {0, 1}, UBlendSpace1D's {0}.
    UPROPERTY()
    TArray<int32> DimensionIndices;

    explicit UBlendSpace(const FObjectInitializer& ObjectInitializer);
    const TArray<FBlendSample>& GetBlendSamples() const { return SampleData; }
    const FBlendParameter& GetBlendParameter(int32 Index) const { return BlendParameters[Index]; }
    virtual EBlendSpaceAxis GetAxisToScale() const { return AxisToScaleAnimation; }
};

}
