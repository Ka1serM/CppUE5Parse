#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "DynamicBlueprintBinding.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UDynamicBlueprintBinding : public UObject {
    GENERATED_BODY()
public:
    virtual void BindDynamicDelegates(UObject* InInstance) const {}
};

}
