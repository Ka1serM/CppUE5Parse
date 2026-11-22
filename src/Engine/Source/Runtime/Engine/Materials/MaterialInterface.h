#pragma once

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/MaterialCachedData.h"
#include "Engine/Source/Runtime/Engine/MaterialTypes.h"

#include "MaterialInterface.generated.h"

namespace ue5parse::runtime {

class UMaterial;
class UTexture;
class UPhysicalMaterial;
class UPhysicalMaterialMask;
class UNeuralProfile;
class USpecularProfile;
class USubsurfaceProfile;
class FMaterialResource;

USTRUCT()
struct FLightmassMaterialInterfaceSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bCastShadowAsMasked = false;
    UPROPERTY()
    float EmissiveBoost = 1.0f;
    UPROPERTY()
    float DiffuseBoost = 1.0f;
    UPROPERTY()
    float ExportResolutionScale = 1.0f;
    UPROPERTY()
    float DistanceFieldPenumbraScale = 1.0f;
    UPROPERTY()
    bool bOverrideCastShadowAsMasked = false;
    UPROPERTY()
    bool bOverrideEmissiveBoost = false;
    UPROPERTY()
    bool bOverrideDiffuseBoost = false;
    UPROPERTY()
    bool bOverrideExportResolutionScale = false;
    UPROPERTY()
    bool bOverrideDistanceFieldPenumbraScale = false;
};

USTRUCT()
struct FMaterialTextureInfo {
    GENERATED_BODY()
    UPROPERTY()
    float SamplingScale = 0;
    UPROPERTY()
    int32 UVChannelIndex = INDEX_NONE;
    UPROPERTY()
    FName TextureName;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMaterialTextureInfo> : TStructOpsTypeTraitsBase2<FMaterialTextureInfo> {
    enum { WithSerializer = true };
};

UCLASS(Abstract)
class UMaterialInterface : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FLightmassMaterialInterfaceSettings LightmassSettings;
    UPROPERTY()
    TObjectPtr<USubsurfaceProfile> SubsurfaceProfile = nullptr;
    UPROPERTY()
    TArray<FMaterialTextureInfo> TextureStreamingData;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    TArray<TObjectPtr<USpecularProfile>> SpecularProfiles;
    UPROPERTY()
    bool bIncludedInBaseGame = false;
    UPROPERTY()
    TObjectPtr<UNeuralProfile> NeuralProfile = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<USubsurfaceProfile>> SubsurfaceProfiles;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> ReferencedDefaultTextures;

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
    virtual bool GetMaterialLayers(FMaterialLayersFunctions& OutLayers) const { return false; }
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

    virtual const FMaterialCachedExpressionData* GetCachedExpressionData() const { return CachedExpressionData; }

    Result<TArray<FMaterialResource>> LoadMaterialResources() const;

protected:
    void SkipInlineShaderMaps(FArchive& Ar);
    void SerializeCachedExpressionData(FArchive& Ar);

    bool GetCachedParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, const FProperty*& OutProperty,
                                 const void*& OutValue) const;
    static bool GetCachedTextureValue(const FProperty* Property, const void* Value, UTexture*& OutValue);
    static bool HasCachedExpressionData();

    FMaterialCachedExpressionData* CachedExpressionData = nullptr;
    int64 OffsetToFirstResource = INDEX_NONE;
};

}
