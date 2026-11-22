#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NetDriver.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UNetDriver : public UObject {
    GENERATED_BODY()
};

}
