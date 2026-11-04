#pragma once

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/FieldPath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"

#include "AttributeSet.generated.h"

namespace ue5parse::runtime {

class UAbilitySystemComponent;
class UAttributeSet;

//! The virtual destructor is part of the cooked layout: the values follow a vtable.
USTRUCT()
struct FGameplayAttributeData {
    GENERATED_BODY()
public:
    FGameplayAttributeData() = default;
    explicit FGameplayAttributeData(float DefaultValue) : BaseValue(DefaultValue), CurrentValue(DefaultValue) {}
    virtual ~FGameplayAttributeData() = default;

    float GetCurrentValue() const { return CurrentValue; }
    virtual void SetCurrentValue(float NewValue) { CurrentValue = NewValue; }
    virtual void SetBaseValue(float NewValue) { BaseValue = NewValue; }

protected:
    UPROPERTY()
    float BaseValue = 0;
    UPROPERTY()
    float CurrentValue = 0;
};

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
    float GetNumericValue(const UAttributeSet* Src) const;

    UPROPERTY()
    FString AttributeName;
    UPROPERTY()
    FFieldPath Attribute;
    UPROPERTY()
    TObjectPtr<UStruct> AttributeOwner = nullptr;
};

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

UCLASS()
class UAttributeSet : public UObject {
    GENERATED_BODY()
public:
    void InitFromMetaDataTable(const UDataTable* DataTable);
};

}
