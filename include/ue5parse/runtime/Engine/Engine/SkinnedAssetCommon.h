#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/MeshUVChannelInfo.h"

#include "SkinnedAssetCommon.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

// FSkeletalMaterial: a material slot of a skeletal mesh (UE's Engine/SkinnedAssetCommon.h).
USTRUCT()
struct FSkeletalMaterial {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialInterface = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    FMeshUVChannelInfo UVChannelData;
    // Editor data; the cook keeps it only when asked.
    FName ImportedMaterialSlotName;

    friend FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Material);
};

}
