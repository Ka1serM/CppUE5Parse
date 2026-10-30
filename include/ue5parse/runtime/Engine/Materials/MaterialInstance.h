#pragma once

#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"
#include "ue5parse/runtime/Engine/StaticParameterSet.h"

#include "MaterialInstance.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FScalarParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    float ParameterValue = 0;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FVectorParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    FLinearColor ParameterValue;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FDoubleVectorParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    FVector4d ParameterValue;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FTextureParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    UTexture* ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

// UMaterialInstance: a material with a parent and parameter overrides (UE's Materials/MaterialInstance.h).
UCLASS(Abstract)
class UMaterialInstance : public UMaterialInterface {
    GENERATED_BODY()
public:
    UPROPERTY()
    UPhysicalMaterial* PhysMaterial = nullptr;
    UPROPERTY()
    UMaterialInterface* Parent = nullptr;
    UPROPERTY()
    bool bHasStaticPermutationResource = false;
    UPROPERTY()
    TArray<FScalarParameterValue> ScalarParameterValues;
    UPROPERTY()
    TArray<FVectorParameterValue> VectorParameterValues;
    UPROPERTY()
    TArray<FDoubleVectorParameterValue> DoubleVectorParameterValues;
    UPROPERTY()
    TArray<FTextureParameterValue> TextureParameterValues;

    void Serialize(FArchive& Ar) override;
    const void* GetCachedExpressionData() const override;
    UMaterial* GetMaterial() override;
    const UMaterial* GetMaterial() const override;
    bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const override;
    bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const override;
    bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const override;
    bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const override;
    bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const override;
    TEnumAsByte<EBlendMode> GetBlendMode() const override;
    bool IsTwoSided() const override;
    bool IsMasked() const override;
    float GetOpacityMaskClipValue() const override;
    EMaterialShadingModel GetShadingModel() const override;
    UPhysicalMaterial* GetPhysicalMaterial() const override;
    // The static switches this instance overrides (StaticParametersRuntime.StaticSwitchParameters).
    TArray<FStaticSwitchParameter> GetStaticSwitchParameters() const;

protected:
    template <class TValue, class FindOverride>
    bool GetInstanceParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, TValue& OutValue,
                                   bool bOveriddenOnly, FindOverride&& Find) const;
    // A BasePropertyOverrides member when its bOverride_ flag is set.
    template <class T>
    bool GetBasePropertyOverride(FName OverrideFlag, FName Member, T& OutValue) const;
};

}
