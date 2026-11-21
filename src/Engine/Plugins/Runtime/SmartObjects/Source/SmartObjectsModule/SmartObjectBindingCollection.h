#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingBindingCollection.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectDefinitionBindableStructDescriptor.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectDefinitionPropertyBinding.h"

#include "SmartObjectBindingCollection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartObjectBindingCollection : public FPropertyBindingBindingCollection {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSmartObjectDefinitionBindableStructDescriptor> BindableStructs;
    UPROPERTY()
    TArray<FSmartObjectDefinitionPropertyBinding> PropertyBindings;
};

}
