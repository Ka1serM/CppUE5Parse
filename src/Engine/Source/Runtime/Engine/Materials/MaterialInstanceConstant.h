#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialInstance.h"

#include "MaterialInstanceConstant.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialInstanceConstant : public UMaterialInstance {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    float K2_GetScalarParameterValue(FName ParameterName);
    UFUNCTION(BlueprintCallable)
    UTexture* K2_GetTextureParameterValue(FName ParameterName);
    UFUNCTION(BlueprintCallable)
    FLinearColor K2_GetVectorParameterValue(FName ParameterName);
};

}
