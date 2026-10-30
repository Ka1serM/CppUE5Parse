#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "BlueprintFunctionLibrary.generated.h"

namespace ue5parse::runtime {

// UBlueprintFunctionLibrary: the base of the classes whose static functions Blueprints call (UE's
// Kismet/BlueprintFunctionLibrary.h).
UCLASS(Abstract)
class UBlueprintFunctionLibrary : public UObject {
    GENERATED_BODY()
};

}
