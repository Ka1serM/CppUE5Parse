#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingBinding.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTypes.h"

#include "SmartObjectDefinitionPropertyBinding.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartObjectDefinitionPropertyBinding : public FPropertyBindingBinding {
    GENERATED_BODY()

    //! Before 5.6 the binding does not derive from FPropertyBindingBinding and stores its own paths.
    UPROPERTY()
    FPropertyBindingPath SourcePath;
    UPROPERTY()
    FPropertyBindingPath TargetPath;
    UPROPERTY()
    FSmartObjectDefinitionDataHandle SourceDataHandle;
    UPROPERTY()
    FSmartObjectDefinitionDataHandle TargetDataHandle;
};

}
