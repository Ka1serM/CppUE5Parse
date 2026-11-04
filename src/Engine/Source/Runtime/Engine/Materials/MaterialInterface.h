#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/MaterialTypes.h"

#include "MaterialInterface.generated.h"

namespace ue5parse::runtime {

class UMaterial;
class UTexture;
class UPhysicalMaterial;
class UPhysicalMaterialMask;
class FMaterialResource;

UCLASS(Abstract)
class UMaterialInterface : public UObject {
    GENERATED_BODY()
public:
    ~UMaterialInterface() override;
    void Serialize(FArchive& Ar) override;

    virtual UMaterial* GetMaterial() { return nullptr; }
    virtual const UMaterial* GetMaterial() const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    UMaterial* GetBaseMaterial() { return GetMaterial(); }

    virtual bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const;
    virtual bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const;
    bool GetScalarParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue) const;
    void GetAllParameterInfoOfType(EMaterialParameterType Type, TArray<FMaterialParameterInfo>& OutParameterInfo) const;

    UFUNCTION(BlueprintCallable)
    virtual TEnumAsByte<EBlendMode> GetBlendMode() const;
    virtual bool IsTwoSided() const;
    virtual bool IsMasked() const;
    virtual float GetOpacityMaskClipValue() const;
    virtual EMaterialShadingModel GetShadingModel() const;

    UFUNCTION(BlueprintCallable)
    virtual UPhysicalMaterial* GetPhysicalMaterial() const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    void SetForceMipLevelsToBeResident(bool OverrideForceMiplevelsToBeResident, bool bForceMiplevelsToBeResidentValue, float ForceDuration,
                                       int32 CinematicTextureGroups, bool bFastResponse) {}

    static const UScriptStruct* GetCachedExpressionDataStruct();
    virtual const void* GetCachedExpressionData() const { return CachedExpressionData; }

    Result<TArray<FMaterialResource>> LoadMaterialResources() const;

protected:
    void SkipInlineShaderMaps(FArchive& Ar);

    bool GetCachedParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, const FProperty*& OutProperty,
                                 const void*& OutValue) const;

    uint8* CachedExpressionData = nullptr;
    int64 OffsetToFirstResource = INDEX_NONE;
};

}
