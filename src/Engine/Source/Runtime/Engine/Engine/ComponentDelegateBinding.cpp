#include "Engine/Source/Runtime/Engine/Engine/ComponentDelegateBinding.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

void UComponentDelegateBinding::BindDynamicDelegates(UObject* InInstance) const {
    for (const FBlueprintComponentDelegateBinding& Binding : ComponentDelegateBindings) {
        FObjectPropertyBase* ComponentProperty = FindFProperty<FObjectPropertyBase>(InInstance->GetClass(), Binding.ComponentPropertyName);
        if (ComponentProperty == nullptr)
            continue;
        UObject* Component = ComponentProperty->GetObjectPropertyValue_InContainer(InInstance);
        if (Component == nullptr)
            continue;
        FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(Component->GetClass(), Binding.DelegatePropertyName);
        if (DelegateProperty == nullptr || InInstance->FindFunction(Binding.FunctionNameToBind) == nullptr)
            continue;
        FScriptDelegate Delegate;
        Delegate.BindUFunction(InInstance, Binding.FunctionNameToBind);
        DelegateProperty->AddDelegate(Delegate, Component, DelegateProperty->ContainerPtrToValuePtr<void>(Component));
    }
}

}
