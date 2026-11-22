#include "Engine/Plugins/Wwise/Source/AkAudio/AkAudioBank.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

void UAkAudioBank::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || HasAnyFlags(RF_ClassDefaultObject) || Ar.GetRemaining() <= 4)
        return;
    switch (Ar.GetGame()) {
    case EGame::GAME_HogwartsLegacy:
    case EGame::GAME_Farlight84:
    case EGame::GAME_ArenaBreakoutInfinite:
    case EGame::GAME_LittleNightmares3:
        return;
    //! Fate Trigger and Century: Age of Ashes save one property of the bank without a tag instead of the cooked data.
    case EGame::GAME_FateTrigger:
        SerializeRawProperty(Ar, 4);
        return;
    case EGame::GAME_CenturyAgeofAshes:
        SerializeRawProperty(Ar, 2);
        return;
    default:
        break;
    }
    const FStructProperty* Property = FindFProperty<FStructProperty>(GetClass(), FName("SoundBankCookedData"));
    if (Property == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UAkAudioBank.SoundBankCookedData");
        return;
    }
    Property->Struct->SerializeItem(Ar, Property->ContainerPtrToValuePtr<void>(this));
}

void UAkAudioBank::SerializeRawProperty(FArchive& Ar, int32 SchemaIndex) {
    const TArray<UStruct::FUnversionedSlot>& Slots = StaticClass()->OwnUnversionedSlots;
    if (!Slots.IsValidIndex(SchemaIndex) || Slots[SchemaIndex].Property == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UAkAudioBank.RawProperty");
        return;
    }
    const UStruct::FUnversionedSlot& Slot = Slots[SchemaIndex];
    const bool bRawProperties = Ar.UseRawPropertySerialization();
    Ar.SetUseRawPropertySerialization(true);
    Slot.Property->SerializeItem(Ar, Slot.Property->ContainerPtrToValuePtr<void>(this, Slot.ArrayIndex));
    Ar.SetUseRawPropertySerialization(bRawProperties);
}

}
