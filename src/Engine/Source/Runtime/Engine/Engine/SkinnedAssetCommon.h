#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/MeshUVChannelInfo.h"

#include "SkinnedAssetCommon.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

USTRUCT()
struct FSkeletalMaterial {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialInterface = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    FMeshUVChannelInfo UVChannelData;
    FName ImportedMaterialSlotName;

    friend FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Material);
};

}
