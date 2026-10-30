#pragma once

#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/FieldPath.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/Engine/Engine/DataTable.h"
#include "ue5parse/runtime/Engine/EngineNativeMissing.h"

#include "AttributeSet.generated.h"

namespace ue5parse::runtime {

class UAbilitySystemComponent;
class UAttributeSet;

// FGameplayAttributeData: an attribute's base value and its current value with the temporary modifiers (UE's
// GameplayAbilities/AttributeSet.h). The virtual destructor is part of the cooked layout: the values follow a vtable.
USTRUCT()
struct FGameplayAttributeData {
    GENERATED_BODY()
public:
    FGameplayAttributeData() = default;
    explicit FGameplayAttributeData(float DefaultValue) : BaseValue(DefaultValue), CurrentValue(DefaultValue) {}
    virtual ~FGameplayAttributeData() = default;

    float GetCurrentValue() const { return CurrentValue; }
    virtual void SetCurrentValue(float NewValue) { CurrentValue = NewValue; }
    float GetBaseValue() const { return BaseValue; }
    virtual void SetBaseValue(float NewValue) { BaseValue = NewValue; }

protected:
    UPROPERTY()
    float BaseValue = 0;
    UPROPERTY()
    float CurrentValue = 0;
};

// FGameplayAttribute: a float or FGameplayAttributeData property of an attribute set.
USTRUCT()
struct FGameplayAttribute {
    GENERATED_BODY()
public:
    FGameplayAttribute() = default;
    explicit FGameplayAttribute(FProperty* NewProperty);

    bool IsValid() const { return GetUProperty() != nullptr; }
    FProperty* GetUProperty() const;
    UClass* GetAttributeSetClass() const;
    static bool IsGameplayAttributeDataProperty(const FProperty* Property);
    // The current value of the attribute in the set.
    float GetNumericValue(const UAttributeSet* Src) const;

    UPROPERTY()
    FString AttributeName;
    UPROPERTY()
    FFieldPath Attribute;
    UPROPERTY()
    TObjectPtr<UStruct> AttributeOwner = nullptr;
};

// FAttributeMetaData: a data table row of an attribute's starting value, named "<Set>.<Attribute>".
USTRUCT()
struct FAttributeMetaData : public FTableRowBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    float BaseValue = 0;
    UPROPERTY()
    float MinValue = 0;
    UPROPERTY()
    float MaxValue = 1;
    UPROPERTY()
    FString DerivedAttributeInfo;
    UPROPERTY()
    bool bCanStack = false;
};

// UAttributeSet: the attributes of an ability system component, as float or FGameplayAttributeData properties. Its
// outer is the owning actor.
UCLASS()
class UAttributeSet : public UObject {
    GENERATED_BODY()
public:
    // Each numeric or FGameplayAttributeData property takes the BaseValue of its row in the table.
    void InitFromMetaDataTable(const UDataTable* DataTable);
};

}
