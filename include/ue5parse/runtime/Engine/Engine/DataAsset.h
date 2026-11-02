#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "DataAsset.generated.h"

namespace ue5parse::runtime {

// UDataAsset and UPrimaryDataAsset: the bases of the assets that hold only data (UE's Engine/DataAsset.h). Their
// properties come from the mappings.
UCLASS(Abstract)
class UDataAsset : public UObject {
    GENERATED_BODY()
};

UCLASS(Abstract)
class UPrimaryDataAsset : public UDataAsset {
    GENERATED_BODY()
};

}
