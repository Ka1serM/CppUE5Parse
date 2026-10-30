#pragma once

#include <cstring>
#include <new>

#include "ue5parse/runtime/Core/Containers/ScriptSet.h"
#include "ue5parse/runtime/Core/Internationalization/Text.h"
#include "ue5parse/runtime/CoreUObject/UObject/Field.h"
#include "ue5parse/runtime/CoreUObject/UObject/FieldPath.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"

namespace ue5parse::runtime {

class FArchive;
class UEnum;
class UFunction;
class UScriptStruct;

// FProperty: a typed slot of a struct, class or function frame, at an offset that UStruct::Link assigns. The value
// operations are UE's: initialize, destroy, copy, compare and hash a value in memory.
class FProperty : public FField {
    DECLARE_FIELD(FProperty, FField, CASTCLASS_FProperty)
public:
    FProperty(FFieldClass* InClass, FFieldVariant InOwner, FName InName, EObjectFlags InFlags)
        : FField(InClass, InOwner, InName, InFlags) {}

    int32 ArrayDim = 1;
    int32 ElementSize = 0;
    EPropertyFlags PropertyFlags = CPF_None;
    uint16 RepIndex = 0;
    uint8 BlueprintReplicationCondition = 0;
    int32 Offset_Internal = 0;
    FName RepNotifyFunc;
    // The struct's chains of properties: all, the ones holding object references, the ones that need a destructor,
    // and the ones that need a constructor.
    FProperty* PropertyLinkNext = nullptr;
    FProperty* NextRef = nullptr;
    FProperty* DestructorLinkNext = nullptr;
    FProperty* PostConstructLinkNext = nullptr;

    int32 GetSize() const { return ArrayDim * ElementSize; }
    int32 GetOffset_ForInternal() const { return Offset_Internal; }
    bool HasAnyPropertyFlags(uint64 Flags) const { return (PropertyFlags & Flags) != 0; }
    bool HasAllPropertyFlags(uint64 Flags) const { return (PropertyFlags & Flags) == Flags; }
    bool IsInContainer(int32 ContainerSize) const { return Offset_Internal + GetSize() <= ContainerSize; }

    // Sets ElementSize and the computed flags (LinkInternal), and returns the size.
    int32 Link() {
        LinkInternal();
        return GetSize();
    }
    // Places the property after the owner's current PropertiesSize; returns the end of the property.
    int32 SetupOffset(int32 OwnerPropertiesSize);

    template <class T = void>
    T* ContainerPtrToValuePtr(void* Container, int32 ArrayIndex = 0) const {
        return reinterpret_cast<T*>(static_cast<uint8*>(Container) + Offset_Internal + ElementSize * ArrayIndex);
    }
    template <class T = void>
    const T* ContainerPtrToValuePtr(const void* Container, int32 ArrayIndex = 0) const {
        return reinterpret_cast<const T*>(static_cast<const uint8*>(Container) + Offset_Internal + ElementSize * ArrayIndex);
    }

    virtual void LinkInternal() {}
    virtual int32 GetMinAlignment() const { return 1; }
    virtual bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const = 0;
    virtual uint32 GetValueTypeHashInternal(const void* Src) const;
    virtual void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const;
    virtual void ClearValueInternal(void* Data) const;
    virtual void DestroyValueInternal(void* Dest) const;
    virtual void InitializeValueInternal(void* Dest) const;
    virtual bool SameType(const FProperty* Other) const;
    // Reads one cooked value into Value (UE's SerializeItem on load), as a container element or struct member writes
    // it. A read that fails sets the archive's error.
    virtual void SerializeItem(FArchive& Ar, void* Value) const = 0;

    // The VM copies values through these, so that a bitfield bool reads and writes as a C++ bool.
    virtual void CopySingleValueToScriptVM(void* Dest, const void* Src) const { CopySingleValue(Dest, Src); }
    virtual void CopyCompleteValueToScriptVM(void* Dest, const void* Src) const { CopyCompleteValue(Dest, Src); }
    virtual void CopySingleValueFromScriptVM(void* Dest, const void* Src) const { CopySingleValue(Dest, Src); }
    virtual void CopyCompleteValueFromScriptVM(void* Dest, const void* Src) const { CopyCompleteValue(Dest, Src); }
    void CopyCompleteValueToScriptVM_InContainer(void* OutValue, const void* InContainer) const {
        CopyCompleteValueToScriptVM(OutValue, ContainerPtrToValuePtr<void>(InContainer));
    }
    void CopyCompleteValueFromScriptVM_InContainer(void* OutContainer, const void* InValue) const {
        CopyCompleteValueFromScriptVM(ContainerPtrToValuePtr<void>(OutContainer), InValue);
    }

    void CopySingleValue(void* Dest, const void* Src) const {
        if (Dest == Src) return;
        if (PropertyFlags & CPF_IsPlainOldData) {
            std::memcpy(Dest, Src, ElementSize);
        } else {
            CopyValuesInternal(Dest, Src, 1);
        }
    }
    void CopyCompleteValue(void* Dest, const void* Src) const {
        if (Dest == Src) return;
        if (PropertyFlags & CPF_IsPlainOldData) {
            std::memcpy(Dest, Src, static_cast<std::size_t>(ElementSize) * ArrayDim);
        } else {
            CopyValuesInternal(Dest, Src, ArrayDim);
        }
    }
    void CopyCompleteValue_InContainer(void* Dest, const void* Src) const {
        CopyCompleteValue(ContainerPtrToValuePtr<void>(Dest), ContainerPtrToValuePtr<void>(Src));
    }
    void ClearValue(void* Data) const {
        if (HasAllPropertyFlags(CPF_NoDestructor | CPF_ZeroConstructor)) {
            std::memset(Data, 0, ElementSize);
        } else {
            ClearValueInternal(Data);
        }
    }
    void ClearValue_InContainer(void* Data, int32 ArrayIndex = 0) const { ClearValue(ContainerPtrToValuePtr<void>(Data, ArrayIndex)); }
    void DestroyValue(void* Dest) const {
        if (!(PropertyFlags & CPF_NoDestructor)) DestroyValueInternal(Dest);
    }
    void DestroyValue_InContainer(void* Dest) const { DestroyValue(ContainerPtrToValuePtr<void>(Dest)); }
    void InitializeValue(void* Dest) const {
        if (PropertyFlags & CPF_ZeroConstructor) {
            std::memset(Dest, 0, static_cast<std::size_t>(ElementSize) * ArrayDim);
        } else {
            InitializeValueInternal(Dest);
        }
    }
    void InitializeValue_InContainer(void* Dest) const { InitializeValue(ContainerPtrToValuePtr<void>(Dest)); }
    uint32 GetValueTypeHash(const void* Src) const { return GetValueTypeHashInternal(Src); }

    FProperty* GetNextProperty() const { return static_cast<FProperty*>(Next); }
};

// Whether zeroed memory is a valid default value of a type (UE's TIsZeroConstructType). True for the runtime's value
// types, except those holding an FName: a zeroed FName is not None.
template <class T>
constexpr bool TIsZeroConstructType = true;
template <>
inline constexpr bool TIsZeroConstructType<FName> = false;
template <>
inline constexpr bool TIsZeroConstructType<FSoftObjectPtr> = false;
template <>
inline constexpr bool TIsZeroConstructType<FScriptDelegate> = false;

// The value operations of a property whose memory is one C++ type, as UE's TProperty.
template <class InTCppType, class TBase>
class TProperty : public TBase {
public:
    using TBase::TBase;
    using TCppType = InTCppType;
    static TCppType GetDefaultPropertyValue() { return TCppType(); }

    static TCppType& GetPropertyValuePtrRef(void* Data) { return *static_cast<TCppType*>(Data); }
    static TCppType GetPropertyValue(const void* Data) { return *static_cast<const TCppType*>(Data); }
    static void SetPropertyValue(void* Data, const TCppType& Value) { *static_cast<TCppType*>(Data) = Value; }
    TCppType GetPropertyValue_InContainer(const void* Container, int32 ArrayIndex = 0) const {
        return GetPropertyValue(this->template ContainerPtrToValuePtr<void>(Container, ArrayIndex));
    }
    void SetPropertyValue_InContainer(void* Container, const TCppType& Value, int32 ArrayIndex = 0) const {
        SetPropertyValue(this->template ContainerPtrToValuePtr<void>(Container, ArrayIndex), Value);
    }

    void LinkInternal() override {
        this->ElementSize = sizeof(TCppType);
        EPropertyFlags Flags = this->PropertyFlags & ~CPF_ComputedFlags;
        if (std::is_trivially_copyable_v<TCppType>) Flags = Flags | CPF_IsPlainOldData | CPF_NoDestructor;
        if (TIsZeroConstructType<TCppType>) Flags = Flags | CPF_ZeroConstructor;
        this->PropertyFlags = Flags | CPF_HasGetValueTypeHash;
    }
    int32 GetMinAlignment() const override { return alignof(TCppType); }
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override {
        for (int32 i = 0; i < Count; i++) static_cast<TCppType*>(Dest)[i] = static_cast<const TCppType*>(Src)[i];
    }
    void ClearValueInternal(void* Data) const override { *static_cast<TCppType*>(Data) = TCppType(); }
    void InitializeValueInternal(void* Dest) const override {
        for (int32 i = 0; i < this->ArrayDim; i++) new (static_cast<TCppType*>(Dest) + i) TCppType();
    }
    void DestroyValueInternal(void* Dest) const override {
        for (int32 i = 0; i < this->ArrayDim; i++) static_cast<TCppType*>(Dest)[i].~TCppType();
    }
    bool Identical(const void* A, const void* B, uint32 = 0) const override {
        if (B == nullptr) return GetPropertyValue(A) == TCppType();
        return GetPropertyValue(A) == GetPropertyValue(B);
    }
};

// FNumericProperty: the integer and floating point properties.
class FNumericProperty : public FProperty {
    DECLARE_FIELD(FNumericProperty, FProperty, CASTCLASS_FNumericProperty)
public:
    using FProperty::FProperty;

    virtual bool IsFloatingPoint() const = 0;
    virtual bool IsInteger() const = 0;
    // The enum of a byte property that has one.
    virtual UEnum* GetIntPropertyEnum() const { return nullptr; }
    bool IsEnum() const { return GetIntPropertyEnum() != nullptr; }
    virtual void SetIntPropertyValue(void* Data, uint64 Value) const = 0;
    virtual void SetIntPropertyValue(void* Data, int64 Value) const = 0;
    virtual void SetFloatingPointPropertyValue(void* Data, double Value) const = 0;
    virtual int64 GetSignedIntPropertyValue(const void* Data) const = 0;
    virtual uint64 GetUnsignedIntPropertyValue(const void* Data) const = 0;
    virtual double GetFloatingPointPropertyValue(const void* Data) const = 0;
};

template <class TCppType>
class TProperty_Numeric : public TProperty<TCppType, FNumericProperty> {
public:
    using TProperty<TCppType, FNumericProperty>::TProperty;

    bool IsFloatingPoint() const override { return std::is_floating_point_v<TCppType>; }
    bool IsInteger() const override { return std::is_integral_v<TCppType>; }
    void SetIntPropertyValue(void* Data, uint64 Value) const override { *static_cast<TCppType*>(Data) = static_cast<TCppType>(Value); }
    void SetIntPropertyValue(void* Data, int64 Value) const override { *static_cast<TCppType*>(Data) = static_cast<TCppType>(Value); }
    void SetFloatingPointPropertyValue(void* Data, double Value) const override {
        *static_cast<TCppType*>(Data) = static_cast<TCppType>(Value);
    }
    int64 GetSignedIntPropertyValue(const void* Data) const override { return static_cast<int64>(*static_cast<const TCppType*>(Data)); }
    uint64 GetUnsignedIntPropertyValue(const void* Data) const override {
        return static_cast<uint64>(*static_cast<const TCppType*>(Data));
    }
    double GetFloatingPointPropertyValue(const void* Data) const override {
        return static_cast<double>(*static_cast<const TCppType*>(Data));
    }
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

#define DECLARE_NUMERIC_PROPERTY(TClass, TCppType, TCastFlags)                                                         \
    class TClass : public TProperty_Numeric<TCppType> {                                                                \
        DECLARE_FIELD(TClass, FNumericProperty, TCastFlags)                                                            \
    public:                                                                                                            \
        TClass(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)                                 \
            : TProperty_Numeric<TCppType>(StaticClass(), InOwner, InName, InFlags) {}                                  \
    };

DECLARE_NUMERIC_PROPERTY(FInt8Property, int8, CASTCLASS_FInt8Property)
DECLARE_NUMERIC_PROPERTY(FInt16Property, int16, CASTCLASS_FInt16Property)
DECLARE_NUMERIC_PROPERTY(FIntProperty, int32, CASTCLASS_FIntProperty)
DECLARE_NUMERIC_PROPERTY(FInt64Property, int64, CASTCLASS_FInt64Property)
DECLARE_NUMERIC_PROPERTY(FUInt16Property, uint16, CASTCLASS_FUInt16Property)
DECLARE_NUMERIC_PROPERTY(FUInt32Property, uint32, CASTCLASS_FUInt32Property)
DECLARE_NUMERIC_PROPERTY(FUInt64Property, uint64, CASTCLASS_FUInt64Property)
DECLARE_NUMERIC_PROPERTY(FFloatProperty, float, CASTCLASS_FFloatProperty)
DECLARE_NUMERIC_PROPERTY(FDoubleProperty, double, CASTCLASS_FDoubleProperty)

// FByteProperty: a uint8, or an enum stored in a byte (TEnumAsByte).
class FByteProperty : public TProperty_Numeric<uint8> {
    DECLARE_FIELD(FByteProperty, FNumericProperty, CASTCLASS_FByteProperty)
public:
    FByteProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty_Numeric<uint8>(StaticClass(), InOwner, InName, InFlags) {}

    UEnum* Enum = nullptr;
    UEnum* GetIntPropertyEnum() const override { return Enum; }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// FBoolProperty. Blueprint and runtime-laid-out bools take their own storage (a native bool, or a byte with mask 1);
// C++ members are native bools.
class FBoolProperty : public FProperty {
    DECLARE_FIELD(FBoolProperty, FProperty, CASTCLASS_FBoolProperty)
public:
    FBoolProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}

    uint8 FieldSize = 1;
    uint8 ByteOffset = 0;
    uint8 ByteMask = 1;
    uint8 FieldMask = 255;

    void SetBoolSize(uint32 InSize, bool bIsNativeBool);
    bool IsNativeBool() const { return FieldMask == 0xff; }
    bool GetPropertyValue(const void* A) const { return (static_cast<const uint8*>(A)[ByteOffset] & FieldMask) != 0; }
    void SetPropertyValue(void* A, bool Value) const {
        uint8& Byte = static_cast<uint8*>(A)[ByteOffset];
        Byte = static_cast<uint8>((Byte & ~FieldMask) | (Value ? ByteMask : 0));
    }
    bool GetPropertyValue_InContainer(const void* Container, int32 ArrayIndex = 0) const {
        return GetPropertyValue(ContainerPtrToValuePtr<void>(Container, ArrayIndex));
    }
    void SetPropertyValue_InContainer(void* Container, bool Value, int32 ArrayIndex = 0) const {
        SetPropertyValue(ContainerPtrToValuePtr<void>(Container, ArrayIndex), Value);
    }

    void LinkInternal() override;
    int32 GetMinAlignment() const override { return FieldSize; }
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    uint32 GetValueTypeHashInternal(const void* Src) const override { return GetPropertyValue(Src) ? 1u : 0u; }
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override;
    void ClearValueInternal(void* Data) const override { SetPropertyValue(Data, false); }
    void InitializeValueInternal(void* Dest) const override;
    void CopySingleValueToScriptVM(void* Dest, const void* Src) const override;
    void CopyCompleteValueToScriptVM(void* Dest, const void* Src) const override;
    void CopySingleValueFromScriptVM(void* Dest, const void* Src) const override;
    void CopyCompleteValueFromScriptVM(void* Dest, const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// The object reference properties. FObjectPropertyBase holds the class the reference must be of.
class FObjectPropertyBase : public FProperty {
    DECLARE_FIELD(FObjectPropertyBase, FProperty, CASTCLASS_FObjectPropertyBase)
public:
    using FProperty::FProperty;

    UClass* PropertyClass = nullptr;

    virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const = 0;
    virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const = 0;
    UObject* GetObjectPropertyValue_InContainer(const void* Container, int32 ArrayIndex = 0) const {
        return GetObjectPropertyValue(ContainerPtrToValuePtr<void>(Container, ArrayIndex));
    }
    void SetObjectPropertyValue_InContainer(void* Container, UObject* Value, int32 ArrayIndex = 0) const {
        SetObjectPropertyValue(ContainerPtrToValuePtr<void>(Container, ArrayIndex), Value);
    }
    bool SameType(const FProperty* Other) const override;
};

class FObjectProperty : public TProperty<UObject*, FObjectPropertyBase> {
    DECLARE_FIELD(FObjectProperty, FObjectPropertyBase, CASTCLASS_FObjectProperty)
public:
    FObjectProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<UObject*, FObjectPropertyBase>(StaticClass(), InOwner, InName, InFlags) {}
    FObjectProperty(FFieldClass* InClass, FFieldVariant InOwner, FName InName, EObjectFlags InFlags)
        : TProperty<UObject*, FObjectPropertyBase>(InClass, InOwner, InName, InFlags) {}

    UObject* GetObjectPropertyValue(const void* Address) const override { return *static_cast<UObject* const*>(Address); }
    void SetObjectPropertyValue(void* Address, UObject* Value) const override { *static_cast<UObject**>(Address) = Value; }
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FClassProperty : public FObjectProperty {
    DECLARE_FIELD(FClassProperty, FObjectProperty, CASTCLASS_FClassProperty)
public:
    FClassProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FObjectProperty(StaticClass(), InOwner, InName, InFlags) {}

    UClass* MetaClass = nullptr;
};

class FWeakObjectProperty : public TProperty<FWeakObjectPtr, FObjectPropertyBase> {
    DECLARE_FIELD(FWeakObjectProperty, FObjectPropertyBase, CASTCLASS_FWeakObjectProperty)
public:
    FWeakObjectProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FWeakObjectPtr, FObjectPropertyBase>(StaticClass(), InOwner, InName, InFlags) {}

    UObject* GetObjectPropertyValue(const void* Address) const override { return static_cast<const FWeakObjectPtr*>(Address)->Get(); }
    void SetObjectPropertyValue(void* Address, UObject* Value) const override { *static_cast<FWeakObjectPtr*>(Address) = Value; }
    uint32 GetValueTypeHashInternal(const void* Src) const override { return static_cast<const FWeakObjectPtr*>(Src)->GetTypeHash(); }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// FLazyObjectProperty: cooked as a 16-byte unique object guid, which cooked games do not resolve; kept as a weak
// pointer that stays null.
class FLazyObjectProperty : public TProperty<FWeakObjectPtr, FObjectPropertyBase> {
    DECLARE_FIELD(FLazyObjectProperty, FObjectPropertyBase, CASTCLASS_FLazyObjectProperty)
public:
    FLazyObjectProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FWeakObjectPtr, FObjectPropertyBase>(StaticClass(), InOwner, InName, InFlags) {}

    UObject* GetObjectPropertyValue(const void* Address) const override { return static_cast<const FWeakObjectPtr*>(Address)->Get(); }
    void SetObjectPropertyValue(void* Address, UObject* Value) const override { *static_cast<FWeakObjectPtr*>(Address) = Value; }
    uint32 GetValueTypeHashInternal(const void* Src) const override { return static_cast<const FWeakObjectPtr*>(Src)->GetTypeHash(); }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FSoftObjectProperty : public TProperty<FSoftObjectPtr, FObjectPropertyBase> {
    DECLARE_FIELD(FSoftObjectProperty, FObjectPropertyBase, CASTCLASS_FSoftObjectProperty)
public:
    FSoftObjectProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FSoftObjectPtr, FObjectPropertyBase>(StaticClass(), InOwner, InName, InFlags) {}
    FSoftObjectProperty(FFieldClass* InClass, FFieldVariant InOwner, FName InName, EObjectFlags InFlags)
        : TProperty<FSoftObjectPtr, FObjectPropertyBase>(InClass, InOwner, InName, InFlags) {}

    UObject* GetObjectPropertyValue(const void* Address) const override { return static_cast<const FSoftObjectPtr*>(Address)->Get(); }
    void SetObjectPropertyValue(void* Address, UObject* Value) const override {
        *static_cast<FSoftObjectPtr*>(Address) = FSoftObjectPtr(Value);
    }
    uint32 GetValueTypeHashInternal(const void* Src) const override {
        return GetTypeHash(static_cast<const FSoftObjectPtr*>(Src)->GetUniqueID());
    }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FSoftClassProperty : public FSoftObjectProperty {
    DECLARE_FIELD(FSoftClassProperty, FSoftObjectProperty, CASTCLASS_FSoftClassProperty)
public:
    FSoftClassProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FSoftObjectProperty(StaticClass(), InOwner, InName, InFlags) {}

    UClass* MetaClass = nullptr;
};

class FInterfaceProperty : public TProperty<FScriptInterface, FProperty> {
    DECLARE_FIELD(FInterfaceProperty, FProperty, CASTCLASS_FInterfaceProperty)
public:
    FInterfaceProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FScriptInterface, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    UClass* InterfaceClass = nullptr;
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FNameProperty : public TProperty<FName, FProperty> {
    DECLARE_FIELD(FNameProperty, FProperty, CASTCLASS_FNameProperty)
public:
    FNameProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FName, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FStrProperty : public TProperty<FString, FProperty> {
    DECLARE_FIELD(FStrProperty, FProperty, CASTCLASS_FStrProperty)
public:
    FStrProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FString, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    uint32 GetValueTypeHashInternal(const void* Src) const override { return GetTypeHash(*static_cast<const FString*>(Src)); }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FTextProperty : public TProperty<FText, FProperty> {
    DECLARE_FIELD(FTextProperty, FProperty, CASTCLASS_FTextProperty)
public:
    FTextProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FText, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    // UE's FTextProperty::Identical_Implementation: the same text data, or equal display strings.
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    uint32 GetValueTypeHashInternal(const void*) const override { return 0; }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FStructProperty : public FProperty {
    DECLARE_FIELD(FStructProperty, FProperty, CASTCLASS_FStructProperty)
public:
    FStructProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}

    UScriptStruct* Struct = nullptr;

    void LinkInternal() override;
    int32 GetMinAlignment() const override;
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override;
    void ClearValueInternal(void* Data) const override;
    void DestroyValueInternal(void* Dest) const override;
    void InitializeValueInternal(void* Dest) const override;
    bool SameType(const FProperty* Other) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FArrayProperty : public FProperty {
    DECLARE_FIELD(FArrayProperty, FProperty, CASTCLASS_FArrayProperty)
public:
    FArrayProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}
    ~FArrayProperty() override { delete Inner; }

    FProperty* Inner = nullptr;

    void LinkInternal() override;
    int32 GetMinAlignment() const override { return alignof(FScriptArray); }
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override;
    void ClearValueInternal(void* Data) const override;
    void DestroyValueInternal(void* Dest) const override;
    bool SameType(const FProperty* Other) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FSetProperty : public FProperty {
    DECLARE_FIELD(FSetProperty, FProperty, CASTCLASS_FSetProperty)
public:
    FSetProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}
    ~FSetProperty() override { delete ElementProp; }

    FProperty* ElementProp = nullptr;
    FScriptSetLayout SetLayout;

    void LinkInternal() override;
    int32 GetMinAlignment() const override { return alignof(FScriptSet); }
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override;
    void ClearValueInternal(void* Data) const override;
    void DestroyValueInternal(void* Dest) const override;
    bool SameType(const FProperty* Other) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FMapProperty : public FProperty {
    DECLARE_FIELD(FMapProperty, FProperty, CASTCLASS_FMapProperty)
public:
    FMapProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}
    ~FMapProperty() override {
        delete KeyProp;
        delete ValueProp;
    }

    FProperty* KeyProp = nullptr;
    FProperty* ValueProp = nullptr;
    FScriptMapLayout MapLayout;

    void LinkInternal() override;
    int32 GetMinAlignment() const override { return alignof(FScriptMap); }
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override;
    void CopyValuesInternal(void* Dest, const void* Src, int32 Count) const override;
    void ClearValueInternal(void* Data) const override;
    void DestroyValueInternal(void* Dest) const override;
    bool SameType(const FProperty* Other) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// FEnumProperty: an enum class value stored in its underlying numeric property.
class FEnumProperty : public FProperty {
    DECLARE_FIELD(FEnumProperty, FProperty, CASTCLASS_FEnumProperty)
public:
    FEnumProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : FProperty(StaticClass(), InOwner, InName, InFlags) {}
    ~FEnumProperty() override { delete UnderlyingProp; }

    FNumericProperty* UnderlyingProp = nullptr;
    UEnum* Enum = nullptr;

    FNumericProperty* GetUnderlyingProperty() const { return UnderlyingProp; }
    UEnum* GetEnum() const { return Enum; }
    void LinkInternal() override;
    int32 GetMinAlignment() const override { return UnderlyingProp->GetMinAlignment(); }
    bool Identical(const void* A, const void* B, uint32 PortFlags = 0) const override { return UnderlyingProp->Identical(A, B, PortFlags); }
    uint32 GetValueTypeHashInternal(const void* Src) const override { return UnderlyingProp->GetValueTypeHash(Src); }
    bool SameType(const FProperty* Other) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FDelegateProperty : public TProperty<FScriptDelegate, FProperty> {
    DECLARE_FIELD(FDelegateProperty, FProperty, CASTCLASS_FDelegateProperty)
public:
    FDelegateProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FScriptDelegate, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    UFunction* SignatureFunction = nullptr;
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// The multicast delegate properties. Sparse delegates of native classes are kept inline too: the runtime lays out
// native classes itself.
class FMulticastDelegateProperty : public FProperty {
    DECLARE_FIELD(FMulticastDelegateProperty, FProperty, CASTCLASS_FMulticastDelegateProperty)
public:
    using FProperty::FProperty;

    UFunction* SignatureFunction = nullptr;

    virtual const FMulticastScriptDelegate* GetMulticastDelegate(const void* PropertyValue) const = 0;
    virtual void SetMulticastDelegate(void* PropertyValue, FMulticastScriptDelegate ScriptDelegate) const = 0;
    virtual void AddDelegate(FScriptDelegate ScriptDelegate, UObject* Parent, void* PropertyValue) const = 0;
    virtual void RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject* Parent, void* PropertyValue) const = 0;
    virtual void ClearDelegate(UObject* Parent, void* PropertyValue) const = 0;
};

class FMulticastInlineDelegateProperty : public TProperty<FMulticastScriptDelegate, FMulticastDelegateProperty> {
    DECLARE_FIELD(FMulticastInlineDelegateProperty, FMulticastDelegateProperty, CASTCLASS_FMulticastInlineDelegateProperty)
public:
    FMulticastInlineDelegateProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FMulticastScriptDelegate, FMulticastDelegateProperty>(StaticClass(), InOwner, InName, InFlags) {}

    const FMulticastScriptDelegate* GetMulticastDelegate(const void* PropertyValue) const override {
        return static_cast<const FMulticastScriptDelegate*>(PropertyValue);
    }
    void SetMulticastDelegate(void* PropertyValue, FMulticastScriptDelegate ScriptDelegate) const override {
        *static_cast<FMulticastScriptDelegate*>(PropertyValue) = std::move(ScriptDelegate);
    }
    void AddDelegate(FScriptDelegate ScriptDelegate, UObject* Parent, void* PropertyValue) const override;
    void RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject* Parent, void* PropertyValue) const override;
    void ClearDelegate(UObject* Parent, void* PropertyValue) const override;
    uint32 GetValueTypeHashInternal(const void*) const override { return 0; }
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

class FFieldPathProperty : public TProperty<FFieldPath, FProperty> {
    DECLARE_FIELD(FFieldPathProperty, FProperty, CASTCLASS_FFieldPathProperty)
public:
    FFieldPathProperty(FFieldVariant InOwner, FName InName, EObjectFlags InFlags = RF_NoFlags)
        : TProperty<FFieldPath, FProperty>(StaticClass(), InOwner, InName, InFlags) {}

    FFieldClass* PropertyClass = nullptr;
    uint32 GetValueTypeHashInternal(const void* Src) const override;
    void SerializeItem(FArchive& Ar, void* Value) const override;
};

// FScriptArrayHelper: element access and resizing of an FArrayProperty's value, constructing and destructing
// elements through the inner property.
class FScriptArrayHelper {
public:
    FScriptArrayHelper(const FArrayProperty* InProperty, const void* InArray)
        : FScriptArrayHelper(InProperty->Inner, InArray) {}
    static FScriptArrayHelper CreateHelperFormInnerProperty(const FProperty* InInner, const void* InArray) {
        return FScriptArrayHelper(InInner, InArray);
    }

    int32 Num() const { return HeapArray->Num(); }
    bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < Num(); }
    uint8* GetRawPtr(int32 Index = 0) {
        return Num() == 0 ? nullptr : static_cast<uint8*>(HeapArray->GetData()) + static_cast<std::size_t>(Index) * ElementSize;
    }
    // Adds constructed elements and returns the index of the first.
    int32 AddValues(int32 Count);
    int32 AddValue() { return AddValues(1); }
    // Adds zeroed elements without constructing them.
    int32 AddUninitializedValues(int32 Count) { return HeapArray->Add(Count, ElementSize, ElementAlignment); }
    void InsertValues(int32 Index, int32 Count = 1);
    void RemoveValues(int32 Index, int32 Count = 1);
    void EmptyValues(int32 Slack = 0);
    void EmptyAndAddValues(int32 Count);
    void Resize(int32 NewNum);
    void SwapValues(int32 A, int32 B) { HeapArray->SwapMemory(A, B, ElementSize); }
    void MoveAssign(void* InOtherArray) {
        HeapArray->Empty(0, ElementSize, ElementAlignment);
        HeapArray->MoveAssign(*static_cast<FScriptArray*>(InOtherArray));
    }

private:
    FScriptArrayHelper(const FProperty* InInner, const void* InArray);
    void ConstructItems(int32 Index, int32 Count);
    void DestructItems(int32 Index, int32 Count);

    const FProperty* Inner;
    FScriptArray* HeapArray;
    int32 ElementSize;
    int32 ElementAlignment;
};

// FScriptSetHelper: the elements of an FSetProperty's value. Indices are sparse: test IsValidIndex while iterating to
// GetMaxIndex.
class FScriptSetHelper {
public:
    FScriptSetHelper(const FSetProperty* InProperty, const void* InSet)
        : FScriptSetHelper(InProperty->ElementProp, InProperty->SetLayout, InSet) {}
    static FScriptSetHelper CreateHelperFormElementProperty(const FProperty* InElementProp, const void* InSet);

    int32 Num() const { return Set->Num(); }
    int32 GetMaxIndex() const { return Set->GetMaxIndex(); }
    bool IsValidIndex(int32 Index) const { return Set->IsValidIndex(Index); }
    uint8* GetElementPtr(int32 Index) { return static_cast<uint8*>(Set->GetData(Index, SetLayout)); }
    // The sparse index of the Nth valid element.
    int32 FindInternalIndex(int32 LogicalIndex) const;
    int32 FindElementIndex(const void* ElementToFind) const;
    // Adds a copy unless an equal element is in the set; returns the element's index either way.
    int32 AddElement(const void* ElementToAdd);
    void RemoveAt(int32 Index, int32 Count = 1);
    bool RemoveElement(const void* ElementToRemove);
    void EmptyElements(int32 Slack = 0);
    // Adds a default element without hashing it: Rehash after filling.
    int32 AddDefaultValue_Invalid_NeedsRehash();
    void Rehash();

    const FProperty* ElementProp;

private:
    FScriptSetHelper(const FProperty* InElementProp, const FScriptSetLayout& InLayout, const void* InSet)
        : ElementProp(InElementProp), Set(const_cast<FScriptSet*>(static_cast<const FScriptSet*>(InSet))), SetLayout(InLayout) {}
    void LinkElement(int32 Index, uint32 Hash);

    FScriptSet* Set;
    FScriptSetLayout SetLayout;
};

// FScriptMapHelper: the pairs of an FMapProperty's value.
class FScriptMapHelper {
public:
    FScriptMapHelper(const FMapProperty* InProperty, const void* InMap)
        : FScriptMapHelper(InProperty->KeyProp, InProperty->ValueProp, InProperty->MapLayout, InMap) {}
    static FScriptMapHelper CreateHelperFormInnerProperties(const FProperty* InKeyProp, const FProperty* InValueProp,
                                                           const void* InMap);

    int32 Num() const { return Map->Pairs.Num(); }
    int32 GetMaxIndex() const { return Map->Pairs.GetMaxIndex(); }
    bool IsValidIndex(int32 Index) const { return Map->Pairs.IsValidIndex(Index); }
    uint8* GetPairPtr(int32 Index) { return static_cast<uint8*>(Map->Pairs.GetData(Index, MapLayout.SetLayout)); }
    uint8* GetKeyPtr(int32 Index) { return GetPairPtr(Index); }
    uint8* GetValuePtr(int32 Index) { return GetPairPtr(Index) + MapLayout.ValueOffset; }
    int32 FindInternalIndex(int32 LogicalIndex) const;
    int32 FindMapIndexWithKey(const void* KeyPtr) const;
    // The value of a key, or null.
    uint8* FindValueFromHash(const void* KeyPtr);
    // Sets the value of a key, adding the pair when the key is new.
    void AddPair(const void* KeyPtr, const void* ValuePtr);
    // Adds a default pair for a key (or finds it) and returns the value.
    uint8* FindOrAdd(const void* KeyPtr);
    void RemoveAt(int32 Index, int32 Count = 1);
    bool RemovePair(const void* KeyPtr);
    void EmptyValues(int32 Slack = 0);
    int32 AddDefaultValue_Invalid_NeedsRehash();
    void Rehash();

    const FProperty* KeyProp;
    const FProperty* ValueProp;

private:
    FScriptMapHelper(const FProperty* InKeyProp, const FProperty* InValueProp, const FScriptMapLayout& InLayout, const void* InMap)
        : KeyProp(InKeyProp), ValueProp(InValueProp), Map(const_cast<FScriptMap*>(static_cast<const FScriptMap*>(InMap))),
          MapLayout(InLayout) {}
    void LinkPair(int32 Index, uint32 Hash);

    FScriptMap* Map;
    FScriptMapLayout MapLayout;
};

// A value of a property in its own memory, constructed and destroyed with it (UE's FDefaultConstructedPropertyElement).
class FDefaultConstructedPropertyElement {
public:
    explicit FDefaultConstructedPropertyElement(const FProperty* InProp);
    ~FDefaultConstructedPropertyElement();
    FDefaultConstructedPropertyElement(const FDefaultConstructedPropertyElement&) = delete;
    FDefaultConstructedPropertyElement& operator=(const FDefaultConstructedPropertyElement&) = delete;
    void* GetObjAddress() const { return Obj; }

private:
    const FProperty* Prop;
    void* Obj;
};

// The FProperty class of a property class name ("IntProperty", "ObjectPtrProperty"); null for an unknown name.
FFieldClass* FindPropertyClass(std::string_view Name);
// A new property of a class, without its types resolved.
FProperty* ConstructProperty(FFieldClass* Class, FFieldVariant Owner, FName Name, EObjectFlags Flags = RF_NoFlags);

}
