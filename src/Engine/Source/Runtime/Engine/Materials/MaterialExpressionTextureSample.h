#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionTextureBase.h"

#include "MaterialExpressionTextureSample.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ETextureMipValueMode : int32 { TMVM_None, TMVM_MipLevel, TMVM_MipBias, TMVM_Derivative, TMVM_MAX };

UCLASS()
class UMaterialExpressionTextureSample : public UMaterialExpressionTextureBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Coordinates;
    UPROPERTY()
    FExpressionInput TextureObject;
    UPROPERTY()
    FExpressionInput MipValue;
    UPROPERTY()
    FExpressionInput CoordinatesDX;
    UPROPERTY()
    FExpressionInput CoordinatesDY;
    UPROPERTY()
    FExpressionInput AutomaticViewMipBiasValue;
    UPROPERTY()
    TEnumAsByte<ETextureMipValueMode> MipValueMode = TMVM_None;
    UPROPERTY()
    TEnumAsByte<ESamplerSourceMode> SamplerSource = SSM_FromTextureAsset;
    UPROPERTY()
    bool AutomaticViewMipBias = true;
    UPROPERTY()
    uint8 ConstCoordinate = 0;
    UPROPERTY()
    int32 ConstMipValue = INDEX_NONE;
};

}
