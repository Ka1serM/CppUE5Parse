#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/MaterialTypes.h"

#include "MaterialInterface.generated.h"

namespace ue5parse::runtime {

class UMaterial;
class UTexture;
class UPhysicalMaterial;
class UPhysicalMaterialMask;

// UMaterialInterface: the base of materials and material instances (UE's Materials/MaterialInterface.h). Parameter
// values resolve as UE does: a material instance's override, else its parent's value, down to the base material's
// cached expression data.
UCLASS(Abstract)
class UMaterialInterface : public UObject {
    GENERATED_BODY()
public:
    ~UMaterialInterface() override;
    void Serialize(FArchive& Ar) override;

    // The base material at the root of the parent chain.
    virtual UMaterial* GetMaterial() { return nullptr; }
    virtual const UMaterial* GetMaterial() const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    UMaterial* GetBaseMaterial() { return GetMaterial(); }

    // The value of a parameter: false when no material in the chain has it. bOveriddenOnly stops at the instances.
    virtual bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const;
    // The parameters of a kind that the chain has, in the base material's order.
    void GetAllParameterInfoOfType(EMaterialParameterType Type, TArray<FMaterialParameterInfo>& OutParameterInfo) const;

    UFUNCTION(BlueprintCallable)
    virtual TEnumAsByte<EBlendMode> GetBlendMode() const;
    virtual bool IsTwoSided() const;
    virtual bool IsMasked() const;
    virtual float GetOpacityMaskClipValue() const;
    virtual EMaterialShadingModel GetShadingModel() const;
    // The textures the material samples (FMaterialCachedExpressionData::ReferencedTextures of the base material).
    void GetUsedTextures(TArray<UTexture*>& OutTextures) const;

    UFUNCTION(BlueprintCallable)
    virtual UPhysicalMaterial* GetPhysicalMaterial() const { return nullptr; }

    // FMaterialCachedExpressionData, saved natively after the properties: the parameter defaults of a base material or of
    // an instance with a static permutation, laid out as the game's struct (GetCachedExpressionDataStruct). An instance
    // without its own takes its parent's, as UE. Null when the chain has none.
    static const UScriptStruct* GetCachedExpressionDataStruct();
    virtual const void* GetCachedExpressionData() const { return CachedExpressionData; }

protected:
    // The default of a parameter in the cached expression data: the value's property (the element of "ScalarValues"...)
    // and its address. False when the data has no such parameter (UE's FMaterialCachedExpressionData::GetParameterValue).
    bool GetCachedParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, const FProperty*& OutProperty,
                                 const void*& OutValue) const;

    uint8* CachedExpressionData = nullptr;
};

}
