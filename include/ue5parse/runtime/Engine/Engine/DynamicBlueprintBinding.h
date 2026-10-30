#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "DynamicBlueprintBinding.generated.h"

namespace ue5parse::runtime {

// UDynamicBlueprintBinding: delegates a Blueprint class binds on its instances when they are constructed (UE's
// Engine/DynamicBlueprintBinding.h).
UCLASS(Abstract)
class UDynamicBlueprintBinding : public UObject {
    GENERATED_BODY()
public:
    virtual void BindDynamicDelegates(UObject* InInstance) const {}
};

}
