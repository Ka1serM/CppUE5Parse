#include "GameTypes/FinalFantasy7Rebirth/EndDataObjectBase.h"

#include <cstring>

#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

enum class EEndPropertyType : int32 {
    Bool = 1,
    Byte = 2,
    Int8 = 3,
    UInt16 = 4,
    Int16 = 5,
    UInt = 6,
    Int = 7,
    Int64 = 8,
    Float = 9,
    Str = 10,
    Name = 11,
};

struct FEndKey {
    FName Name;
    int32 Index = 0;
};

struct FEndPropertyDefinition {
    FName Name;
    EEndPropertyType Type = EEndPropertyType::Bool;
    bool bArray = false;
};

EPropertyBagPropertyType GetBagType(EEndPropertyType Type) {
    switch (Type) {
    case EEndPropertyType::Bool:
        return EPropertyBagPropertyType::Bool;
    case EEndPropertyType::Byte:
        return EPropertyBagPropertyType::Byte;
    case EEndPropertyType::Int8:
        return EPropertyBagPropertyType::Int8;
    case EEndPropertyType::UInt16:
        return EPropertyBagPropertyType::UInt16;
    case EEndPropertyType::Int16:
        return EPropertyBagPropertyType::Int16;
    case EEndPropertyType::UInt:
        return EPropertyBagPropertyType::UInt32;
    case EEndPropertyType::Int:
        return EPropertyBagPropertyType::Int32;
    case EEndPropertyType::Int64:
        return EPropertyBagPropertyType::Int64;
    case EEndPropertyType::Float:
        return EPropertyBagPropertyType::Float;
    case EEndPropertyType::Str:
        return EPropertyBagPropertyType::String;
    case EEndPropertyType::Name:
        return EPropertyBagPropertyType::Name;
    }
    return EPropertyBagPropertyType::None;
}

int64 GetArrayElementAlignment(EEndPropertyType Type) {
    switch (Type) {
    case EEndPropertyType::Bool:
    case EEndPropertyType::Byte:
    case EEndPropertyType::Int8:
        return 1;
    case EEndPropertyType::UInt16:
    case EEndPropertyType::Int16:
        return 2;
    default:
        return 4;
    }
}

void ReadScalar(FMemoryMappedImageReader& Reader, EEndPropertyType Type, const FProperty* Property, void* Value) {
    switch (Type) {
    case EEndPropertyType::Bool:
        CastField<FBoolProperty>(Property)->SetPropertyValue(Value, Reader.Read<uint8>() != 0);
        return;
    case EEndPropertyType::Byte:
    case EEndPropertyType::Int8:
        Reader.ReadBytes(Value, 1);
        return;
    case EEndPropertyType::UInt16:
    case EEndPropertyType::Int16:
        Reader.ReadBytes(Value, 2);
        return;
    case EEndPropertyType::UInt:
    case EEndPropertyType::Int:
    case EEndPropertyType::Float:
        Reader.Align(4);
        Reader.ReadBytes(Value, 4);
        return;
    case EEndPropertyType::Int64:
        Reader.ReadBytes(Value, 8);
        return;
    case EEndPropertyType::Str:
        Reader.Align(8);
        *static_cast<FString*>(Value) = Reader.ReadString();
        return;
    case EEndPropertyType::Name:
        Reader.Align(4);
        *static_cast<FName*>(Value) = Reader.ReadName();
        return;
    }
    Reader.SetError();
}

void ReadArray(FMemoryMappedImageReader& Reader, EEndPropertyType Type, const FArrayProperty* Property, void* Value) {
    Reader.Align(8);
    int32 Num = 0;
    const int64 Data = Reader.ReadArrayHeader(Num, true);
    if (Num == 0 || Reader.IsError())
        return;
    const int64 Continue = Reader.Tell();
    Reader.Seek(Data);
    FScriptArrayHelper Helper(Property, Value);
    Helper.Resize(Num);
    for (int32 Index = 0; Index < Num && !Reader.IsError(); Index++) {
        ReadScalar(Reader, Type, Property->Inner, Helper.GetRawPtr(Index));
        Reader.Align(GetArrayElementAlignment(Type));
    }
    Reader.Seek(Continue);
}

void ReadRow(FMemoryMappedImageReader& Reader, const UScriptStruct* Bag, const TArray<FEndPropertyDefinition>& Definitions, FInstancedStruct& Row) {
    Row.InitializeAs(Bag);
    for (const FEndPropertyDefinition& Definition : Definitions) {
        const FProperty* Property = Bag->FindPropertyByName(Definition.Name);
        if (Property == nullptr) {
            Reader.SetError();
            return;
        }
        void* Value = Property->ContainerPtrToValuePtr<void>(Row.GetMutableMemory());
        if (Definition.bArray)
            ReadArray(Reader, Definition.Type, CastField<FArrayProperty>(Property), Value);
        else
            ReadScalar(Reader, Definition.Type, Property, Value);
    }
}

}

void UEndDataObjectBase::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FMemoryMappedImageReader Reader(Image);
    TArray<FEndKey> Keys = Reader.ReadSparseArray<FEndKey>(
        [&Reader] {
            FEndKey Key;
            Key.Name = Reader.ReadName();
            Key.Index = Reader.Read<int32>();
            Reader.Read<int32>();
            Reader.Read<int32>();
            return Key;
        },
        20);
    Reader.ReadRawArray<int32>();
    TArray<FEndPropertyDefinition> Definitions = Reader.ReadArray<FEndPropertyDefinition>([&Reader] {
        FEndPropertyDefinition Definition;
        Definition.Name = Reader.ReadName();
        Definition.Type = static_cast<EEndPropertyType>(Reader.Read<int32>());
        Definition.bArray = Definition.Name.toString().ends_with("_Array");
        if (Definition.Type < EEndPropertyType::Bool || Definition.Type > EEndPropertyType::Name)
            Reader.SetError();
        return Definition;
    });
    if (Reader.IsError()) {
        Ar.SetError(ErrorCode::InvalidData, "UEndDataObjectBase.Definitions");
        return;
    }
    TArray<FPropertyBagPropertyDesc> Descs;
    for (const FEndPropertyDefinition& Definition : Definitions) {
        FPropertyBagPropertyDesc& Desc = Descs.Emplace_GetRef();
        Desc.Name = Definition.Name;
        Desc.ValueType = GetBagType(Definition.Type);
        if (Definition.bArray)
            Desc.ContainerTypes.Add(EPropertyBagContainerType::Array);
    }
    const UScriptStruct* Bag = GetOrCreatePropertyBag(Descs);
    TArray<FInstancedStruct> Rows = Reader.ReadArray<FInstancedStruct>([&] {
        FInstancedStruct Row;
        ReadRow(Reader, Bag, Definitions, Row);
        return Row;
    });
    if (Reader.IsError()) {
        Ar.SetError(ErrorCode::InvalidData, "UEndDataObjectBase.DataTable");
        return;
    }
    for (int32 Index = 0; Index < Keys.Num() && Index < Rows.Num(); Index++)
        DataTable.FindOrAdd(Keys[Index].Name) = Rows[Index];
}

}
