#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/AttributeSet.h"
#include "Engine/Source/Runtime/GameplayTasks/GameplayTasksNative.h"

#include "AbilitySystemComponent.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAttributeDefaults {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<UAttributeSet> Attributes;
    UPROPERTY()
    TObjectPtr<UDataTable> DefaultStartingTable = nullptr;
};

UCLASS()
class UAbilitySystemComponent : public UGameplayTasksComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FAttributeDefaults> DefaultStartingData;
    UPROPERTY(Replicated)
    TArray<TObjectPtr<UAttributeSet>> SpawnedAttributes;

    void OnRegister() override;

    float GetNumericAttribute(const FGameplayAttribute& Attribute) const;
    bool HasAttributeSetForAttribute(const FGameplayAttribute& Attribute) const;
    const UAttributeSet* GetAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass) const;
    const UAttributeSet* GetOrCreateAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass);
};

}
