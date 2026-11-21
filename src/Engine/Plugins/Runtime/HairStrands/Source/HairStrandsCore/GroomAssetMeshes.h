#pragma once

#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetCards.h"

#include "GroomAssetMeshes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FHairGroupsMeshesSourceDescription {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ImportedMesh = nullptr;
    UPROPERTY()
    FHairGroupCardsTextures Textures;
    UPROPERTY()
    int32 GroupIndex = 0;
    UPROPERTY()
    int32 LODIndex = -1;
    UPROPERTY()
    FString ImportedMeshKey;
};

}
