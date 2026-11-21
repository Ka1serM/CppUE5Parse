#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingPath.h"

#include "PropertyBindingBinding.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyBindingBinding {
    GENERATED_BODY()

    UPROPERTY()
    FPropertyBindingPath SourcePropertyPath;
    UPROPERTY()
    FPropertyBindingPath TargetPropertyPath;
};

}
