#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

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

    friend FArchive& operator<<(FArchive& Ar, FPropertyBagPropertyDescMetaData& PropertyDescMetaData) { return Ar << PropertyDescMetaData.Key << PropertyDescMetaData.Value; }
};

struct FPropertyBagPropertyDesc {
    UObject* ValueTypeObject = nullptr;
    FGuid ID;
    FName Name;
    EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::None;
    FPropertyBagContainerTypes ContainerTypes;
    uint64 PropertyFlags = CPF_Edit;
    EPropertyBagPropertyType KeyType = EPropertyBagPropertyType::None;
    UObject* KeyTypeObject = nullptr;
    //! UE keeps the metadata and the meta class only in editor builds; the port keeps them as read.
    TArray<FPropertyBagPropertyDescMetaData> MetaData;
    TObjectPtr<UClass> MetaClass = nullptr;

    void Serialize(FArchive& Ar);
};

class UPropertyBag;

//! The bag is a transient UPropertyBag named PropertyBag_<hash>, and descs with the same hash share it.
const UPropertyBag* GetOrCreatePropertyBag(const TArray<FPropertyBagPropertyDesc>& PropertyDescs);

class UPropertyBag : public UScriptStruct {
    DECLARE_CLASS(UPropertyBag, UScriptStruct, 0)
    explicit UPropertyBag(const FObjectInitializer& ObjectInitializer) : UScriptStruct(ObjectInitializer) {}

public:
    static const UPropertyBag* GetOrCreateFromDescs(const TArray<FPropertyBagPropertyDesc>& InPropertyDescs) { return GetOrCreatePropertyBag(InPropertyDescs); }

    const TArray<FPropertyBagPropertyDesc>& GetPropertyDescs() const { return PropertyDescs; }

protected:
    TArray<FPropertyBagPropertyDesc> PropertyDescs;

    friend const UPropertyBag* GetOrCreatePropertyBag(const TArray<FPropertyBagPropertyDesc>& PropertyDescs);
};

class FInstancedPropertyBag {
public:
    static constexpr int32 GetValueOffset() { return offsetof(FInstancedPropertyBag, Value); }
    const FInstancedStruct& GetValue() const { return Value; }
    const UPropertyBag* GetPropertyBagStruct() const;
    bool operator==(const FInstancedPropertyBag& Other) const { return Value == Other.Value; }
    bool Serialize(FArchive& Ar);

private:
    FInstancedStruct Value;
};

template <>
struct TStructOpsTypeTraits<FInstancedPropertyBag> : TStructOpsTypeTraitsBase2<FInstancedPropertyBag> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
