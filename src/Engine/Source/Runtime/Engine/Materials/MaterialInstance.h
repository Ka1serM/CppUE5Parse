#pragma once

#include "Engine/Source/Runtime/Engine/Engine/BlendableInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceBasePropertyOverrides.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialOverrideNanite.h"
#include "Engine/Source/Runtime/Engine/StaticParameterSet.h"

#include "MaterialInstance.generated.h"

namespace ue5parse::runtime {

class UFont;
class UMaterialParameterCollection;
class URuntimeVirtualTexture;
class USparseVolumeTexture;
class USpecularProfile;
class UTextureCollection;
class UToonProfile;

USTRUCT()
struct FScalarParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    float ParameterValue = 0;
    UPROPERTY()
    FGuid ExpressionGUID;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FScalarParameterValue> : TStructOpsTypeTraitsBase2<FScalarParameterValue> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FVectorParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    FLinearColor ParameterValue;
    UPROPERTY()
    FGuid ExpressionGUID;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FVectorParameterValue> : TStructOpsTypeTraitsBase2<FVectorParameterValue> {
    enum { WithSerializer = true };
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
    FName ParameterName;
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<UTexture> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FTextureParameterValue> : TStructOpsTypeTraitsBase2<FTextureParameterValue> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FFontParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<UFont> FontValue = nullptr;
    UPROPERTY()
    int32 FontPage = 0;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FRuntimeVirtualTextureParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<URuntimeVirtualTexture> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FSparseVolumeTextureParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<USparseVolumeTexture> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FTextureCollectionParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<UTextureCollection> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FParameterCollectionParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FToonProfileParameterValue {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    TObjectPtr<UToonProfile> ParameterValue = nullptr;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FUserSceneTextureOverride {
    GENERATED_BODY()
    UPROPERTY()
    FName Key;
    UPROPERTY()
    FName Value;
};

USTRUCT()
struct FMaterialInstanceCachedData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<int32> ParentLayerIndexRemap;
};

UCLASS(Abstract)
class UMaterialInstance : public UMaterialInterface {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Parent = nullptr;
    UPROPERTY()
    bool bHasStaticPermutationResource = false;
    UPROPERTY()
    FGuid ParentLightingGuid;
    UPROPERTY()
    TArray<FFontParameterValue> FontParameterValues;
    UPROPERTY()
    TArray<FScalarParameterValue> ScalarParameterValues;
    UPROPERTY()
    TArray<FTextureParameterValue> TextureParameterValues;
    UPROPERTY()
    TArray<FVectorParameterValue> VectorParameterValues;
    UPROPERTY()
    bool bOverrideBaseProperties = false;
    UPROPERTY()
    FMaterialInstanceBasePropertyOverrides BasePropertyOverrides;
    UPROPERTY()
    bool bOverrideSubsurfaceProfile = false;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> PermutationTextureReferences;
    UPROPERTY()
    FStaticParameterSet StaticParameters;
    UPROPERTY()
    TArray<FRuntimeVirtualTextureParameterValue> RuntimeVirtualTextureParameterValues;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysicalMaterialMap = nullptr;
    UPROPERTY()
    FMaterialCachedParameters CachedLayerParameters;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> CachedReferencedTextures;
    UPROPERTY()
    TArray<FDoubleVectorParameterValue> DoubleVectorParameterValues;
    UPROPERTY()
    FMaterialOverrideNanite NaniteOverrideMaterial;
    UPROPERTY()
    FStaticParameterSetRuntimeData StaticParametersRuntime;
    UPROPERTY()
    TArray<FSparseVolumeTextureParameterValue> SparseVolumeTextureParameterValues;
    UPROPERTY()
    bool bOverrideBlendableLocation = false;
    UPROPERTY()
    bool bOverrideBlendablePriority = false;
    UPROPERTY()
    TEnumAsByte<EBlendableLocation> BlendableLocationOverride = BL_SceneColorAfterTonemapping;
    UPROPERTY()
    int32 BlendablePriorityOverride = 0;
    UPROPERTY()
    TArray<FTextureCollectionParameterValue> TextureCollectionParameterValues;
    UPROPERTY()
    TArray<FUserSceneTextureOverride> UserSceneTextureOverrides;
    UPROPERTY()
    TObjectPtr<USpecularProfile> SpecularProfileOverride = nullptr;
    UPROPERTY()
    bool bOverrideSpecularProfile = false;
    UPROPERTY()
    TArray<FParameterCollectionParameterValue> ParameterCollectionParameterValues;
    UPROPERTY()
    TObjectPtr<UToonProfile> ToonProfileOverride = nullptr;
    UPROPERTY()
    bool bOverrideToonProfile = false;
    UPROPERTY()
    bool bOverridePhysMaterial = false;
    UPROPERTY()
    TArray<FToonProfileParameterValue> ToonProfileParameterValues;
    FMaterialInstanceCachedData CachedData;

    void Serialize(FArchive& Ar) override;
    const FMaterialCachedExpressionData* GetCachedExpressionData() const override;
    UMaterial* GetMaterial() override;
    const UMaterial* GetMaterial() const override;
    bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const override;
    bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const override;
    bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const override;
    bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const override;
    bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const override;
    bool GetMaterialLayers(FMaterialLayersFunctions& OutLayers) const override;
    TEnumAsByte<EBlendMode> GetBlendMode() const override;
    bool IsTwoSided() const override;
    bool IsMasked() const override;
    float GetOpacityMaskClipValue() const override;
    EMaterialShadingModel GetShadingModel() const override;
    UPhysicalMaterial* GetPhysicalMaterial() const override;
    const TArray<FStaticSwitchParameter>& GetStaticSwitchParameters() const;

protected:
    template <class TValue, class FindOverride>
    bool GetInstanceParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, TValue& OutValue, bool bOveriddenOnly,
                                   FindOverride&& Find) const;
};

}
