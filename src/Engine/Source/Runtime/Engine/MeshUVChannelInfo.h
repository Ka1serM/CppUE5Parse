#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MeshUVChannelInfo.generated.h"

namespace ue5parse::runtime {

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
