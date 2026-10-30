#include "ue5parse/runtime/GameplayAbilities/AttributeSet.h"

#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

// UE's GameplayAbilities/AttributeSet.cpp (UE 5.3).

namespace ue5parse::runtime {

FGameplayAttribute::FGameplayAttribute(FProperty* NewProperty) {
    Attribute = NewProperty;
    AttributeOwner = NewProperty != nullptr ? NewProperty->GetOwnerStruct() : nullptr;
    if (NewProperty != nullptr) AttributeName = NewProperty->GetName();
}

FProperty* FGameplayAttribute::GetUProperty() const { return CastField<FProperty>(Attribute.Get()); }

UClass* FGameplayAttribute::GetAttributeSetClass() const {
    const FProperty* Property = GetUProperty();
    return Property != nullptr ? Property->GetOwnerClass() : nullptr;
}

bool FGameplayAttribute::IsGameplayAttributeDataProperty(const FProperty* Property) {
    const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
    return StructProperty != nullptr && StructProperty->Struct != nullptr &&
           StructProperty->Struct->IsChildOf(FGameplayAttributeData::StaticStruct());
}

float FGameplayAttribute::GetNumericValue(const UAttributeSet* Src) const {
    const FProperty* Property = GetUProperty();
    if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        return static_cast<float>(NumericProperty->GetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Src)));
    if (!IsGameplayAttributeDataProperty(Property)) return 0;
    return Property->ContainerPtrToValuePtr<FGameplayAttributeData>(Src)->GetCurrentValue();
}

void UAttributeSet::InitFromMetaDataTable(const UDataTable* DataTable) {
    if (DataTable->GetRowStruct() == nullptr || !DataTable->GetRowStruct()->IsChildOf(FAttributeMetaData::StaticStruct())) return;
    for (FProperty* Property = GetClass()->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
        if (NumericProperty == nullptr && !FGameplayAttribute::IsGameplayAttributeDataProperty(Property)) continue;
        const FName RowName = MakeName(Property->GetOwnerStruct()->GetName() + FString(".") + Property->GetName());
        const auto* MetaData = reinterpret_cast<const FAttributeMetaData*>(DataTable->FindRowUnchecked(RowName));
        if (MetaData == nullptr) continue;
        if (NumericProperty != nullptr) {
            NumericProperty->SetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this), MetaData->BaseValue);
            continue;
        }
        FGameplayAttributeData* Data = Property->ContainerPtrToValuePtr<FGameplayAttributeData>(this);
        // The cooked values follow a vtable pointer that zeroed memory does not set: no virtual call.
        Data->FGameplayAttributeData::SetBaseValue(MetaData->BaseValue);
        Data->FGameplayAttributeData::SetCurrentValue(MetaData->BaseValue);
    }
}

}
