#pragma once

#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/GameplayAbilities/AttributeSet.h"
#include "ue5parse/runtime/GameplayTasks/GameplayTasksNative.h"

#include "AbilitySystemComponent.generated.h"

namespace ue5parse::runtime {

// FAttributeDefaults: an attribute set class and the table of its starting values.
USTRUCT()
struct FAttributeDefaults {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<UAttributeSet> Attributes;
    UPROPERTY()
    UDataTable* DefaultStartingTable = nullptr;
};

// UAbilitySystemComponent: the attributes of an actor (UE's GameplayAbilities/AbilitySystemComponent.h). Abilities,
// gameplay effects and cues are not ported: the attributes keep their starting values.
UCLASS()
class UAbilitySystemComponent : public UGameplayTasksComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FAttributeDefaults> DefaultStartingData;
    UPROPERTY(Replicated)
    TArray<UAttributeSet*> SpawnedAttributes;

    // Creates an attribute set of each DefaultStartingData entry that has a table, initialized from it.
    void OnRegister() override;

    // The current value of the attribute; 0 when no spawned attribute set holds it.
    float GetNumericAttribute(const FGameplayAttribute& Attribute) const;
    bool HasAttributeSetForAttribute(const FGameplayAttribute& Attribute) const;
    const UAttributeSet* GetAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass) const;
    // The spawned set of the class, or a new one whose outer is the owner.
    const UAttributeSet* GetOrCreateAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass);
};

}
