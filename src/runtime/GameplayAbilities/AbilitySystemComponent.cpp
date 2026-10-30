#include "ue5parse/runtime/GameplayAbilities/AbilitySystemComponent.h"

#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's GameplayAbilities/AbilitySystemComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

void UAbilitySystemComponent::OnRegister() {
    Super::OnRegister();
    for (const FAttributeDefaults& Defaults : DefaultStartingData) {
        if (Defaults.Attributes.Get() == nullptr || Defaults.DefaultStartingTable == nullptr) continue;
        UAttributeSet* Attributes = const_cast<UAttributeSet*>(GetOrCreateAttributeSubobject(Defaults.Attributes));
        if (Attributes != nullptr) Attributes->InitFromMetaDataTable(Defaults.DefaultStartingTable);
    }
}

const UAttributeSet* UAbilitySystemComponent::GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const {
    for (const UAttributeSet* Set : SpawnedAttributes)
        if (Set != nullptr && Set->IsA(AttributeClass)) return Set;
    return nullptr;
}

const UAttributeSet* UAbilitySystemComponent::GetOrCreateAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) {
    AActor* OwningActor = GetOwner();
    if (OwningActor == nullptr || AttributeClass.Get() == nullptr) return nullptr;
    if (const UAttributeSet* Existing = GetAttributeSubobject(AttributeClass)) return Existing;
    UAttributeSet* Attributes = NewObject<UAttributeSet>(OwningActor, AttributeClass);
    SpawnedAttributes.Add(Attributes);
    return Attributes;
}

bool UAbilitySystemComponent::HasAttributeSetForAttribute(const FGameplayAttribute& Attribute) const {
    return Attribute.IsValid() && GetAttributeSubobject(Attribute.GetAttributeSetClass()) != nullptr;
}

float UAbilitySystemComponent::GetNumericAttribute(const FGameplayAttribute& Attribute) const {
    if (!Attribute.IsValid()) return 0;
    const UAttributeSet* AttributeSet = GetAttributeSubobject(Attribute.GetAttributeSetClass());
    return AttributeSet != nullptr ? Attribute.GetNumericValue(AttributeSet) : 0;
}

}
