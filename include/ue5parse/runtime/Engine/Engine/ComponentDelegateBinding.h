#pragma once

#include "ue5parse/runtime/Engine/Engine/DynamicBlueprintBinding.h"

#include "ComponentDelegateBinding.generated.h"

namespace ue5parse::runtime {

// FBlueprintComponentDelegateBinding: a component event a Blueprint handles: the component variable, its delegate, and
// the Blueprint's function.
USTRUCT()
struct FBlueprintComponentDelegateBinding {
    GENERATED_BODY()
    UPROPERTY()
    FName ComponentPropertyName;
    UPROPERTY()
    FName DelegatePropertyName;
    UPROPERTY()
    FName FunctionNameToBind;
};

// UComponentDelegateBinding: the component events of a Blueprint class (UE's Engine/ComponentDelegateBinding.h).
UCLASS()
class UComponentDelegateBinding : public UDynamicBlueprintBinding {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBlueprintComponentDelegateBinding> ComponentDelegateBindings;

    void BindDynamicDelegates(UObject* InInstance) const override;
};

}
