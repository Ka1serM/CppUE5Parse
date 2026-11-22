#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionTextureBase.generated.h"

namespace ue5parse::runtime {

class UTexture;

UCLASS(Abstract)
class UMaterialExpressionTextureBase : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UTexture> Texture = nullptr;
    UPROPERTY()
    TEnumAsByte<EMaterialSamplerType> SamplerType = SAMPLERTYPE_Color;
    UPROPERTY()
    bool IsDefaultMeshpaintTexture = false;
};

}
