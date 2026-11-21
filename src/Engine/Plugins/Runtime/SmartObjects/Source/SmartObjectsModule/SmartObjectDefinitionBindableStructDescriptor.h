#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingBindableStructDescriptor.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTypes.h"

#include "SmartObjectDefinitionBindableStructDescriptor.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartObjectDefinitionBindableStructDescriptor : public FPropertyBindingBindableStructDescriptor {
    GENERATED_BODY()

    UPROPERTY()
    FSmartObjectDefinitionDataHandle DataHandle;
};

}
