#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Engine/MaterialSceneTextureId.h"

#include "MaterialExpressionSceneTexture.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionSceneTexture : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Coordinates;
    UPROPERTY()
    TEnumAsByte<ESceneTextureId> SceneTextureId = PPI_SceneColor;
    UPROPERTY()
    bool bFiltered = false;
};

}
