#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MeshUVChannelInfo.generated.h"

namespace ue5parse::runtime {

// FMeshUVChannelInfo: the texture streaming densities of a material slot.
USTRUCT()
struct FMeshUVChannelInfo {
    GENERATED_BODY()
    UPROPERTY()
    bool bInitialized = false;
    UPROPERTY()
    bool bOverrideDensities = false;
    UPROPERTY()
    float LocalUVDensities[4] = {};
};

FArchive& operator<<(FArchive& Ar, FMeshUVChannelInfo& Info);

}
