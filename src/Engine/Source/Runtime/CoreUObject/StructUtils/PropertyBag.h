#pragma once

#include <optional>

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

namespace ue5parse::runtime {

struct FPropertyBagCustomVersion {
    static constexpr FGuid GUID{0x134A157E, 0xD5E249A3, 0x8D4E843C, 0x98FE9E31};
    enum Type : int32 {
        BeforeCustomVersionWasAdded = 0,
        ContainerTypes = 1,
        NestedContainerTypes = 2,
        MetaClass = 3,
        PropertyFlags = 4,
        KeyTypes = 5,
    };
};

enum class EPropertyBagPropertyType : uint8 {
    None,
    Bool,
    Byte,
    Int32,
    Int64,
    Float,
    Double,
    Name,
    String,
    Text,
    Enum,
    Struct,
    Object,
    SoftObject,
    Class,
    SoftClass,
    Int8,
    Int16,
    UInt16,
    UInt32,
    UInt64,
};

enum class EPropertyBagContainerType : uint8 {
    None,
    Array,
    Set,
    Map,
};

struct FPropertyBagContainerTypes {
    static constexpr uint8 MaxNestedTypes = 2;

    uint8 NumContainers = 0;
    EPropertyBagContainerType Types[MaxNestedTypes] = {EPropertyBagContainerType::None, EPropertyBagContainerType::None};

    void Add(EPropertyBagContainerType Type);
    void Serialize(FArchive& Ar);
};

struct FPropertyBagPropertyDescMetaData {
    FName Key;
    FString Value;
};

struct FPropertyBagPropertyDesc {
    UObject* ValueTypeObject = nullptr;
    FGuid ID;
    FName Name;
    EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::None;
    FPropertyBagContainerTypes ContainerTypes;
    std::optional<TArray<FPropertyBagPropertyDescMetaData>> MetaData;
    UObject* MetaClass = nullptr;
    uint64 PropertyFlags = CPF_Edit;
    EPropertyBagPropertyType KeyType = EPropertyBagPropertyType::None;
    UObject* KeyTypeObject = nullptr;

    void Serialize(FArchive& Ar);
};

//! The bag is a transient UScriptStruct named PropertyBag_<hash>, and descs with the same hash share it.
const UScriptStruct* GetOrCreatePropertyBag(const TArray<FPropertyBagPropertyDesc>& PropertyDescs);

class FInstancedPropertyBag {
public:
    static constexpr int32 GetValueOffset() { return offsetof(FInstancedPropertyBag, Value); }
    const FInstancedStruct& GetValue() const { return Value; }
    bool operator==(const FInstancedPropertyBag& Other) const { return Value == Other.Value; }
    bool Serialize(FArchive& Ar);

protected:
    bool Serialize(FArchive& Ar, TArray<FPropertyBagPropertyDesc>& PropertyDescs, int32& SerialSize);

private:
    FInstancedStruct Value;
};

template <>
struct TStructOpsTypeTraits<FInstancedPropertyBag> : TStructOpsTypeTraitsBase2<FInstancedPropertyBag> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
