#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

class UScriptStruct;

//! Packages older than UE 4.25 keep each property as an export of one of these classes.
class UProperty : public UField {
    DECLARE_CLASS(UProperty, UField, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;
    FProperty* GetAssociatedFField() const { return AssociatedField; }
    void SetAssociatedFField(FProperty* InField) { AssociatedField = InField; }

    int32 ArrayDim = 1;
    EPropertyFlags PropertyFlags = CPF_None;
    FName RepNotifyFunc;
    uint8 BlueprintReplicationCondition = 0;

private:
    FProperty* AssociatedField = nullptr;
};

class UNumericProperty : public UProperty {
    DECLARE_CLASS(UNumericProperty, UProperty, CASTCLASS_None)
    using Super::Super;
};

class UByteProperty : public UNumericProperty {
    DECLARE_CLASS(UByteProperty, UNumericProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UEnum* Enum = nullptr;
};

class UInt8Property : public UNumericProperty {
    DECLARE_CLASS(UInt8Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UInt16Property : public UNumericProperty {
    DECLARE_CLASS(UInt16Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UIntProperty : public UNumericProperty {
    DECLARE_CLASS(UIntProperty, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UInt64Property : public UNumericProperty {
    DECLARE_CLASS(UInt64Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UUInt16Property : public UNumericProperty {
    DECLARE_CLASS(UUInt16Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UUInt32Property : public UNumericProperty {
    DECLARE_CLASS(UUInt32Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UUInt64Property : public UNumericProperty {
    DECLARE_CLASS(UUInt64Property, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UFloatProperty : public UNumericProperty {
    DECLARE_CLASS(UFloatProperty, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UDoubleProperty : public UNumericProperty {
    DECLARE_CLASS(UDoubleProperty, UNumericProperty, CASTCLASS_None)
    using Super::Super;
};

class UBoolProperty : public UProperty {
    DECLARE_CLASS(UBoolProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    uint8 BoolSize = sizeof(uint32);
    bool bNativeBool = false;
};

class UObjectPropertyBase : public UProperty {
    DECLARE_CLASS(UObjectPropertyBase, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UClass* PropertyClass = nullptr;
};

class UObjectProperty : public UObjectPropertyBase {
    DECLARE_CLASS(UObjectProperty, UObjectPropertyBase, CASTCLASS_None)
    using Super::Super;
};

class UWeakObjectProperty : public UObjectPropertyBase {
    DECLARE_CLASS(UWeakObjectProperty, UObjectPropertyBase, CASTCLASS_None)
    using Super::Super;
};

class ULazyObjectProperty : public UObjectPropertyBase {
    DECLARE_CLASS(ULazyObjectProperty, UObjectPropertyBase, CASTCLASS_None)
    using Super::Super;
};

class USoftObjectProperty : public UObjectPropertyBase {
    DECLARE_CLASS(USoftObjectProperty, UObjectPropertyBase, CASTCLASS_None)
    using Super::Super;
};

class UClassProperty : public UObjectProperty {
    DECLARE_CLASS(UClassProperty, UObjectProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UClass* MetaClass = nullptr;
};

class USoftClassProperty : public USoftObjectProperty {
    DECLARE_CLASS(USoftClassProperty, USoftObjectProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UClass* MetaClass = nullptr;
};

class UInterfaceProperty : public UProperty {
    DECLARE_CLASS(UInterfaceProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UClass* InterfaceClass = nullptr;
};

class UNameProperty : public UProperty {
    DECLARE_CLASS(UNameProperty, UProperty, CASTCLASS_None)
    using Super::Super;
};

class UStrProperty : public UProperty {
    DECLARE_CLASS(UStrProperty, UProperty, CASTCLASS_None)
    using Super::Super;
};

class UTextProperty : public UProperty {
    DECLARE_CLASS(UTextProperty, UProperty, CASTCLASS_None)
    using Super::Super;
};

class UArrayProperty : public UProperty {
    DECLARE_CLASS(UArrayProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UProperty* Inner = nullptr;
};

class UMapProperty : public UProperty {
    DECLARE_CLASS(UMapProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UProperty* KeyProp = nullptr;
    UProperty* ValueProp = nullptr;
};

class USetProperty : public UProperty {
    DECLARE_CLASS(USetProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UProperty* ElementProp = nullptr;
};

class UStructProperty : public UProperty {
    DECLARE_CLASS(UStructProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UScriptStruct* Struct = nullptr;
};

class UDelegateProperty : public UProperty {
    DECLARE_CLASS(UDelegateProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UFunction* SignatureFunction = nullptr;
};

class UMulticastDelegateProperty : public UProperty {
    DECLARE_CLASS(UMulticastDelegateProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UFunction* SignatureFunction = nullptr;
};

class UMulticastInlineDelegateProperty : public UMulticastDelegateProperty {
    DECLARE_CLASS(UMulticastInlineDelegateProperty, UMulticastDelegateProperty, CASTCLASS_None)
    using Super::Super;
};

class UMulticastSparseDelegateProperty : public UMulticastDelegateProperty {
    DECLARE_CLASS(UMulticastSparseDelegateProperty, UMulticastDelegateProperty, CASTCLASS_None)
    using Super::Super;
};

class UEnumProperty : public UProperty {
    DECLARE_CLASS(UEnumProperty, UProperty, CASTCLASS_None)
    using Super::Super;

public:
    void Serialize(FArchive& Ar) override;

    UEnum* Enum = nullptr;
    UNumericProperty* UnderlyingProp = nullptr;
};

//! UE's FField::CreateFromUField: the property the legacy export describes, made once and kept on the export.
FProperty* CreateFromUField(UProperty* InField);

}
