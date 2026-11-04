#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "DataAsset.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UDataAsset : public UObject {
    GENERATED_BODY()
};

UCLASS(Abstract)
class UPrimaryDataAsset : public UDataAsset {
    GENERATED_BODY()
};

}
