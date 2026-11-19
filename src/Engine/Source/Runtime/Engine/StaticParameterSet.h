#pragma once

#include "Engine/Source/Runtime/Engine/MaterialTypes.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialLayersFunctions.h"

#include "StaticParameterSet.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    bool bOverride = false;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FStaticSwitchParameter : public FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    bool Value = false;
};

USTRUCT()
struct FStaticComponentMaskParameter : public FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    bool R = false;
    UPROPERTY()
    bool G = false;
    UPROPERTY()
    bool B = false;
    UPROPERTY()
    bool A = false;
};

USTRUCT()
struct FStaticTerrainLayerWeightParameter {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    bool bOverride = false;
    UPROPERTY()
    FGuid ExpressionGUID;
    UPROPERTY()
    int32 WeightmapIndex = INDEX_NONE;
    UPROPERTY()
    bool bWeightBasedBlend = true;
    UPROPERTY()
    FName LayerName;
    UPROPERTY()
    bool bIsRepeatedLayer = false;
};

USTRUCT()
struct FStaticMaterialLayersParameter : public FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialLayersFunctions Value;
};

USTRUCT()
struct FStaticParameterSetRuntimeData {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialLayersFunctionsRuntimeData MaterialLayers;
    UPROPERTY()
    TArray<FStaticSwitchParameter> StaticSwitchParameters;
    UPROPERTY()
    bool bHasMaterialLayers = false;
};

USTRUCT()
struct FStaticParameterSet {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FStaticSwitchParameter> StaticSwitchParameters;
    UPROPERTY()
    TArray<FStaticComponentMaskParameter> StaticComponentMaskParameters;
    UPROPERTY()
    TArray<FStaticTerrainLayerWeightParameter> TerrainLayerWeightParameters;
    UPROPERTY()
    TArray<FStaticMaterialLayersParameter> MaterialLayersParameters;
    UPROPERTY()
    FMaterialLayersFunctions MaterialLayers;
    UPROPERTY()
    bool bHasMaterialLayers = false;
};

}
