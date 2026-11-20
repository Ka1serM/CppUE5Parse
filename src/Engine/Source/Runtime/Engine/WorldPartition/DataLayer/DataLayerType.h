#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "DataLayerType.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EDataLayerType : uint8 {
    Runtime,
    Editor,
    Unknown,
    Size,
};

}
