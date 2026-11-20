#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionTextureSample.h"

#include "MaterialExpressionParticleSubUV.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionParticleSubUV : public UMaterialExpressionTextureSample {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bBlend = true;
};

}
