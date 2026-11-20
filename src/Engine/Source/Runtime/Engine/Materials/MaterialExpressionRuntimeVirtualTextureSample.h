#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Engine/VT/RuntimeVirtualTextureEnum.h"

#include "MaterialExpressionRuntimeVirtualTextureSample.generated.h"

namespace ue5parse::runtime {

class URuntimeVirtualTexture;

UENUM()
enum ERuntimeVirtualTextureMipValueMode : int32 { RVTMVM_None, RVTMVM_MipLevel, RVTMVM_MipBias, RVTMVM_RecalculateDerivatives, RVTMVM_MAX };

UCLASS()
class UMaterialExpressionRuntimeVirtualTextureSample : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Coordinates;
    UPROPERTY()
    FExpressionInput WorldPosition;
    UPROPERTY()
    FExpressionInput MipValue;
    UPROPERTY()
    TObjectPtr<URuntimeVirtualTexture> VirtualTexture = nullptr;
    UPROPERTY()
    ERuntimeVirtualTextureMaterialType MaterialType = ERuntimeVirtualTextureMaterialType::BaseColor;
    UPROPERTY()
    bool bSinglePhysicalSpace = true;
    UPROPERTY()
    TEnumAsByte<ERuntimeVirtualTextureMipValueMode> MipValueMode = RVTMVM_None;
};

}
