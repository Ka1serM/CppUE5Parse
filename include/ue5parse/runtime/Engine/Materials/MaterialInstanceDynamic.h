#pragma once

#include "ue5parse/runtime/Engine/Materials/MaterialInstance.h"

#include "MaterialInstanceDynamic.generated.h"

namespace ue5parse::runtime {

// UMaterialInstanceDynamic: a material instance made at run time whose parameters scripts set (UE's
// Materials/MaterialInstanceDynamic.h).
UCLASS()
class UMaterialInstanceDynamic : public UMaterialInstance {
    GENERATED_BODY()
public:
    // A new instance of a parent material in an outer (UE's Create).
    static UMaterialInstanceDynamic* Create(UMaterialInterface* ParentMaterial, UObject* InOuter, FName Name = FName());

    UFUNCTION(BlueprintCallable)
    void SetScalarParameterValue(FName ParameterName, float Value);
    UFUNCTION(BlueprintCallable)
    void SetScalarParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, float Value);
    UFUNCTION(BlueprintCallable)
    void SetVectorParameterValue(FName ParameterName, FLinearColor Value);
    UFUNCTION(BlueprintCallable)
    void SetVectorParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, FLinearColor Value);
    UFUNCTION(BlueprintCallable)
    void SetDoubleVectorParameterValue(FName ParameterName, FVector4 Value);
    UFUNCTION(BlueprintCallable)
    void SetTextureParameterValue(FName ParameterName, UTexture* Value);
    UFUNCTION(BlueprintCallable)
    void SetTextureParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, UTexture* Value);
    UFUNCTION(BlueprintCallable)
    float K2_GetScalarParameterValue(FName ParameterName);
    UFUNCTION(BlueprintCallable)
    float K2_GetScalarParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo);
    UFUNCTION(BlueprintCallable)
    UTexture* K2_GetTextureParameterValue(FName ParameterName);
    UFUNCTION(BlueprintCallable)
    UTexture* K2_GetTextureParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo);
    UFUNCTION(BlueprintCallable)
    FLinearColor K2_GetVectorParameterValue(FName ParameterName);
    UFUNCTION(BlueprintCallable)
    FLinearColor K2_GetVectorParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo);
    // Copies the overrides of another instance (UE's CopyParameterOverrides).
    UFUNCTION(BlueprintCallable)
    void CopyParameterOverrides(UMaterialInstance* MaterialInstance);
    UFUNCTION(BlueprintCallable)
    void K2_CopyMaterialInstanceParameters(UMaterialInterface* Source, bool bQuickParametersOnly);
    UFUNCTION(BlueprintCallable)
    void ClearParameterValues();
    UFUNCTION(BlueprintCallable)
    void K2_InterpolateMaterialInstanceParams(UMaterialInstance* SourceA, UMaterialInstance* SourceB, float Alpha);
};

}
