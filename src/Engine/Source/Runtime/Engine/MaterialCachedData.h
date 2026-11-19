#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/Set.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/MaterialTypes.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialLayersFunctions.h"

#include "MaterialCachedData.generated.h"

namespace ue5parse::runtime {

class UClass;
class UFont;
class ULandscapeGrassType;
class UMaterialCacheVirtualTextureTag;
class UMaterialFunctionInterface;
class UMaterialParameterCollection;
class URuntimeVirtualTexture;
class USparseVolumeTexture;
class UTexture;
class UTextureCollection;
class UToonProfile;

USTRUCT()
struct FMaterialFunctionInfo {
    GENERATED_BODY()
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    TObjectPtr<UMaterialFunctionInterface> Function = nullptr;
};

USTRUCT()
struct FMaterialParameterCollectionInfo {
    GENERATED_BODY()
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> ParameterCollection = nullptr;
};

USTRUCT()
struct FMaterialCachedParameterEntry {
    GENERATED_BODY()

    int32 FindParameterIndex(const FMaterialParameterInfo& ParameterInfo) const;
    void GetParameterInfos(TArray<FMaterialParameterInfo>& OutParameterInfo) const;

    UPROPERTY()
    TArray<uint64> NameHashes;
    UPROPERTY()
    TArray<FMaterialParameterInfo> ParameterInfos;
    UPROPERTY()
    TArray<FGuid> ExpressionGuids;
    UPROPERTY()
    TArray<bool> Overrides;
    UPROPERTY()
    TSet<FMaterialParameterInfo> ParameterInfoSet;
};

USTRUCT()
struct FMaterialCachedParameters {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialCachedParameterEntry Entries[NumMaterialRuntimeParameterTypes];
    UPROPERTY()
    FMaterialCachedParameterEntry RuntimeEntries[NumMaterialRuntimeParameterTypes];
    UPROPERTY()
    TArray<int32> ScalarPrimitiveDataIndexValues;
    UPROPERTY()
    TArray<int32> VectorPrimitiveDataIndexValues;
    UPROPERTY()
    TArray<float> ScalarValues;
    UPROPERTY()
    TArray<FLinearColor> VectorValues;
    UPROPERTY()
    TArray<FVector4d> DoubleVectorValues;
    UPROPERTY()
    TArray<TObjectPtr<UTexture>> TextureValues;
    UPROPERTY()
    TArray<TObjectPtr<UFont>> FontValues;
    UPROPERTY()
    TArray<int32> FontPageValues;
    UPROPERTY()
    TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextureValues;
};

USTRUCT()
struct FMaterialCachedExpressionData {
    GENERATED_BODY()

    const UStruct* GetParametersStruct() const;
    const void* GetParameters() const;
    const FMaterialCachedParameterEntry* GetParameterTypeEntry(EMaterialParameterType Type) const;

    UPROPERTY()
    FMaterialCachedParameters Parameters;
    UPROPERTY()
    FMaterialCachedParameterEntry RuntimeEntries[NumMaterialRuntimeParameterTypes];
    UPROPERTY()
    TArray<int32> ScalarPrimitiveDataIndexValues;
    UPROPERTY()
    TArray<int32> VectorPrimitiveDataIndexValues;
    UPROPERTY()
    TArray<float> ScalarValues;
    UPROPERTY()
    TArray<bool> StaticSwitchValues;
    UPROPERTY()
    TArray<bool> DynamicSwitchValues;
    UPROPERTY()
    TArray<FLinearColor> VectorValues;
    UPROPERTY()
    TArray<FVector4d> DoubleVectorValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<UTexture>> TextureValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<UTextureCollection>> TextureCollectionValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<UMaterialParameterCollection>> ParameterCollectionValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<UFont>> FontValues;
    UPROPERTY()
    TArray<int32> FontPageValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextureValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<USparseVolumeTexture>> SparseVolumeTextureValues;
    UPROPERTY()
    TArray<TSoftObjectPtr<UToonProfile>> ToonProfileValues;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> ReferencedTextures;
    UPROPERTY()
    TArray<TObjectPtr<UTextureCollection>> ReferencedTextureCollections;
    UPROPERTY()
    TArray<TObjectPtr<UClass>> ReferencedExternalCodeExpressionClasses;
    UPROPERTY()
    TArray<FMaterialFunctionInfo> FunctionInfos;
    UPROPERTY()
    uint32 FunctionInfosStateCRC = 0;
    UPROPERTY()
    TArray<FMaterialParameterCollectionInfo> ParameterCollectionInfos;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialFunctionInterface>> DefaultLayers;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialFunctionInterface>> DefaultLayerBlends;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeGrassType>> GrassTypes;
    UPROPERTY()
    TMap<FName, TObjectPtr<ULandscapeGrassType>> NamedGrassTypes;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialCacheVirtualTextureTag>> MaterialCacheTags;
    UPROPERTY()
    FMaterialLayersFunctionsRuntimeData MaterialLayers;
    UPROPERTY()
    TArray<FName> DynamicParameterNames;
    UPROPERTY()
    TArray<bool> QualityLevelsUsed;
    UPROPERTY()
    uint64 MaterialCacheUVCoordinatesUsedMask = 0;
    UPROPERTY()
    uint32 bHasMaterialLayers : 1 = 0;
    UPROPERTY()
    uint32 bHasRuntimeVirtualTextureOutput : 1 = 0;
    UPROPERTY()
    uint32 bHasFirstPersonOutput : 1 = 0;
    UPROPERTY()
    uint32 bUsesTemporalResponsiveness : 1 = 0;
    UPROPERTY()
    uint32 bUsesMotionVectorWorldOffset : 1 = 0;
    UPROPERTY()
    uint32 bSamplesMaterialCache : 1 = 0;
    UPROPERTY()
    uint32 bHasMaterialCacheOutput : 1 = 0;
    UPROPERTY()
    uint32 bMaterialCacheHasNonUVDerivedExpression : 1 = 0;
    UPROPERTY()
    uint32 bHasSceneColor : 1 = 0;
    UPROPERTY()
    uint32 bHasPerInstanceCustomData : 1 = 0;
    UPROPERTY()
    uint32 bHasPerInstanceRandom : 1 = 0;
    UPROPERTY()
    uint32 bHasVertexInterpolator : 1 = 0;
    UPROPERTY()
    uint32 bHasCustomizedUVs : 1 = 0;
    UPROPERTY()
    uint32 bHasMeshPaintTexture : 1 = 0;
    UPROPERTY()
    uint32 bHasWorldPosition : 1 = 0;
    UPROPERTY()
    uint32 MaterialAttributesPropertyConnectedBitmask = 0;
    UPROPERTY()
    uint32 PropertyConnectedBitmask = 0;
    UPROPERTY()
    uint64 PropertyConnectedMask = 0;
};

}
