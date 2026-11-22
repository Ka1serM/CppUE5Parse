#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

#include "MaterialExpressionLandscapeLayerBlend.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ELandscapeLayerBlendType : int32 { LB_WeightBlend, LB_AlphaBlend, LB_HeightBlend };

USTRUCT()
struct FLayerBlendInput {
    GENERATED_BODY()
    UPROPERTY()
    FName LayerName;
    UPROPERTY()
    TEnumAsByte<ELandscapeLayerBlendType> BlendType = LB_WeightBlend;
    UPROPERTY()
    FExpressionInput LayerInput;
    UPROPERTY()
    FExpressionInput HeightInput;
    UPROPERTY()
    float PreviewWeight = 0.0f;
    UPROPERTY()
    FVector ConstLayerInput = FVector::ZeroVector;
    UPROPERTY()
    float ConstHeightInput = 0.0f;
};

UCLASS()
class UMaterialExpressionLandscapeLayerBlend : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FLayerBlendInput> Layers;
    UPROPERTY()
    FGuid ExpressionGUID;
};

}
