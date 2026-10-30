#include <string>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"

// Unversioned property serialization (UE's UnversionedPropertySerialization.cpp): a header of fragments says which
// indices of the struct's schema have a value and which values are zero; the values follow in schema order.

namespace ue5parse::runtime {

namespace {

// FUnversionedHeader fragment: 7 bits skip count, 1 bit "has zeroes", 1 bit "is last", 7 bits value count.
struct FFragment {
    uint8 SkipNum;
    bool bHasAnyZeroes;
    bool bIsLast;
    uint8 ValueNum;
};

FFragment UnpackFragment(uint16 Packed) {
    return FFragment{static_cast<uint8>(Packed & 0x7f), (Packed & 0x80) != 0, (Packed & 0x100) != 0, static_cast<uint8>(Packed >> 9)};
}

constexpr int32 MaxFragments = 256;
constexpr int32 MaxZeroMaskWords = 64;

// One value at the top level of a struct. Enums and enum bytes are their number here; containers and struct members
// inside them go through SerializeItem (names).
void SerializeTopLevelValue(FArchive& Ar, const FProperty* Property, void* Value) {
    if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property)) {
        Enum->GetUnderlyingProperty()->SerializeItem(Ar, Value);
        return;
    }
    if (Property->IsA<FByteProperty>()) {
        Ar.Serialize(Value, 1);
        return;
    }
    Property->SerializeItem(Ar, Value);
}

}

void UStruct::BuildUnversionedSchema() {
    UnversionedSchema = OwnUnversionedSlots;
    // A function's frame is not serialized, and does not inherit its super's layout.
    UStruct* InheritanceSuper = IsA<UFunction>() ? nullptr : GetSuperStruct();
    if (InheritanceSuper != nullptr)
        for (const FUnversionedSlot& Slot : InheritanceSuper->UnversionedSchema) UnversionedSchema.Add(Slot);
}

void UStruct::SerializeTaggedProperties(FArchive& Ar, uint8* Data) const {
    if (!Ar.UseUnversionedPropertySerialization()) {
        // Cooked Zen packages of a game are unversioned; tagged properties only come from editor data.
        Ar.SetError(ErrorCode::UnsupportedVersion, "SerializeTaggedProperties.Tagged");
        return;
    }

    // The header: fragments until the last one, then the zero mask of the fragments that have zeroes.
    FFragment Fragments[MaxFragments];
    int32 FragmentCount = 0;
    uint32 ZeroMaskBits = 0;
    uint32 ValueCount = 0;
    for (;;) {
        if (FragmentCount == MaxFragments) {
            Ar.SetError(ErrorCode::InvalidData, "SerializeUnversionedProperties.Fragments");
            return;
        }
        uint16 Packed = 0;
        Ar << Packed;
        if (Ar.IsError()) return;
        FFragment Fragment = UnpackFragment(Packed);
        Fragments[FragmentCount++] = Fragment;
        ValueCount += Fragment.ValueNum;
        if (Fragment.bHasAnyZeroes) ZeroMaskBits += Fragment.ValueNum;
        if (Fragment.bIsLast) break;
    }
    uint32 ZeroMask[MaxZeroMaskWords] = {};
    if (ZeroMaskBits > 0) {
        if (ZeroMaskBits <= 8) {
            uint8 Word = 0;
            Ar << Word;
            ZeroMask[0] = Word;
        } else if (ZeroMaskBits <= 16) {
            uint16 Word = 0;
            Ar << Word;
            ZeroMask[0] = Word;
        } else {
            uint32 Words = (ZeroMaskBits + 31) / 32;
            if (Words > MaxZeroMaskWords) {
                Ar.SetError(ErrorCode::InvalidData, "SerializeUnversionedProperties.ZeroMask");
                return;
            }
            for (uint32 i = 0; i < Words; i++) Ar << ZeroMask[i];
        }
        if (Ar.IsError()) return;
    }
    if (ValueCount == 0) return;

    // The values, in schema order.
    uint32 SchemaIndex = 0;
    uint32 ZeroIndex = 0;
    for (int32 f = 0; f < FragmentCount; f++) {
        const FFragment& Fragment = Fragments[f];
        SchemaIndex += Fragment.SkipNum;
        for (uint32 v = 0; v < Fragment.ValueNum; v++, SchemaIndex++) {
            bool bZero = false;
            if (Fragment.bHasAnyZeroes) {
                bZero = ((ZeroMask[ZeroIndex / 32] >> (ZeroIndex % 32)) & 1) != 0;
                ZeroIndex++;
            }
            const FUnversionedSlot* Slot = UnversionedSchema.IsValidIndex(static_cast<int32>(SchemaIndex)) ? &UnversionedSchema[SchemaIndex] : nullptr;
            if (Slot == nullptr || Slot->Property == nullptr) {
                // A zero value has no bytes, so a property the runtime does not have can be passed; a value cannot.
                if (bZero) continue;
                ReportLoadProblem("load.schema_slot", GetName() + FString(" slot ") + FString(std::to_string(SchemaIndex)) +
                                                          FString(" of ") + FString(std::to_string(UnversionedSchema.Num())));
                Ar.SetError(ErrorCode::MissingProperty, "SerializeUnversionedProperties.UnknownProperty");
                return;
            }
            void* Value = Slot->Property->ContainerPtrToValuePtr<void>(Data, Slot->ArrayIndex);
            if (bZero) {
                Slot->Property->ClearValue(Value);
            } else {
                SerializeTopLevelValue(Ar, Slot->Property, Value);
                if (Ar.IsError()) return;
            }
        }
    }
}

void UScriptStruct::SerializeItem(FArchive& Ar, void* Value) const {
    if (CppStructOps != nullptr && CppStructOps->Serialize != nullptr && CppStructOps->Serialize(Ar, Value)) return;
    SerializeTaggedProperties(Ar, static_cast<uint8*>(Value));
}

}
