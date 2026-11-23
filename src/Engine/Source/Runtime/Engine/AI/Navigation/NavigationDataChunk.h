#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NavigationDataChunk.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UNavigationDataChunk : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName NavigationDataName;
};

}
