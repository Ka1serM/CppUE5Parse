#pragma once

#include "Engine/Source/Runtime/Engine/Engine/DynamicBlueprintBinding.h"

#include "ComponentDelegateBinding.generated.h"

namespace ue5parse::runtime {

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

UCLASS()
class UComponentDelegateBinding : public UDynamicBlueprintBinding {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBlueprintComponentDelegateBinding> ComponentDelegateBindings;

    void BindDynamicDelegates(UObject* InInstance) const override;
};

}
