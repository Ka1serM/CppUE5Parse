#pragma once

#include <mutex>
#include <new>
#include <type_traits>

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

class FArchive;

// UField: the base of structs, functions and enums. Children of a struct (its functions) link through Next.
class UField : public UObject {
    DECLARE_CLASS(UField, UObject, CASTCLASS_UField)
    explicit UField(const FObjectInitializer& ObjectInitializer);
public:
    UField* Next = nullptr;
};

// UStruct: a struct, a class or a function. Its own properties (ChildProperties), its functions (Children), its
// bytecode in memory format, and the layout that Link computes: offsets of its properties after its super's.
class UStruct : public UField {
    DECLARE_CLASS(UStruct, UField, CASTCLASS_UStruct)
    explicit UStruct(const FObjectInitializer& ObjectInitializer);
public:
    ~UStruct() override;

    UStruct* GetSuperStruct() const { return SuperStruct; }
    void SetSuperStruct(UStruct* NewSuperStruct) { SuperStruct = NewSuperStruct; }
    bool IsChildOf(const UStruct* SomeBase) const;
    template <class T>
    bool IsChildOf() const {
        return IsChildOf(T::StaticClass());
    }
    int32 GetPropertiesSize() const { return PropertiesSize; }
    int32 GetMinAlignment() const { return MinAlignment; }
    int32 GetStructureSize() const { return (PropertiesSize + MinAlignment - 1) / MinAlignment * MinAlignment; }
    bool IsLinked() const { return bLinked; }
    // The property of the name on this struct and its supers.
    FProperty* FindPropertyByName(FName InName) const;
    // Appends a property to ChildProperties (before Link).
    void AddCppProperty(FProperty* Property);

    // Computes the layout: links each own property after the super's PropertiesSize, then builds the property chains.
    virtual void Link();
    // The property chains (PropertyLink, RefLink, DestructorLink, PostConstructLink) over the linked properties;
    // marks the struct linked. Native types that keep C++ offsets lay out themselves and call only this.
    void BuildPropertyChains();
    // Constructs the properties of ArrayDim structs at Dest that need it (UE's InitializeStruct). Dest is zeroed.
    virtual void InitializeStruct(void* Dest, int32 ArrayDim = 1) const;
    virtual void DestroyStruct(void* Dest, int32 ArrayDim = 1) const;
    // Reads serialized properties into Data, laid out as this struct (UE's SerializeTaggedProperties on load). Cooked
    // packages are unversioned: UnversionedPropertySerialization.cpp.
    void SerializeTaggedProperties(FArchive& Ar, uint8* Data) const;
    // Reads the cooked struct: its super, children, ChildProperties (FField serialization) and bytecode.
    void Serialize(FArchive& Ar) override;

    // One index of the unversioned property schema: a property and the element of its static array, or a gap (a
    // property the mappings list without a type).
    struct FUnversionedSlot {
        FProperty* Property = nullptr;
        int32 ArrayIndex = 0;
    };
    // The schema of this struct's own properties, in serialization order: ChildProperties times ArrayDim for a cooked
    // struct, the mappings' indices for a native one.
    TArray<FUnversionedSlot> OwnUnversionedSlots;
    // The own slots, then the super struct's schema (UE's FUnversionedStructSchema).
    TArray<FUnversionedSlot> UnversionedSchema;
    void BuildUnversionedSchema();

    UStruct* SuperStruct = nullptr;
    UField* Children = nullptr;
    FField* ChildProperties = nullptr;
    int32 PropertiesSize = 0;
    int32 MinAlignment = 1;
    // The bytecode in memory format (see Script.h), and the names its 12-byte name slots index.
    TArray<uint8> Script;
    TArray<FName> ScriptNames;
    // The bytecode as cooked, and its size in memory format; turned into Script on first use.
    TArray<uint8> SerializedScript;
    int32 BytecodeBufferSize = 0;
    FProperty* PropertyLink = nullptr;
    FProperty* RefLink = nullptr;
    FProperty* DestructorLink = nullptr;
    FProperty* PostConstructLink = nullptr;
    // The properties of PropertyLink by name, for FindPropertyByName: equal names keep the chain's order.
    TArray<FProperty*> PropertiesByName;

protected:
    // Turns the cooked bytecode into the memory format, resolving its objects, properties and names (UE's
    // SerializeExpr on load). Runs on the first call of a function, once its owner class is linked.
    friend class FScriptLinker;
    bool bLinked = false;
    bool bScriptLinked = false;
};

// The C++ operations of a native struct: its size and how to construct, destruct, copy, compare and hash it, and how
// to read it from cooked data when the cook writes it natively (FVector, FGuid). UE's ICppStructOps.
struct FCppStructOps {
    int32 Size = 0;
    int32 Alignment = 1;
    void (*Construct)(void* Dest) = nullptr;
    void (*Destruct)(void* Dest) = nullptr;
    void (*Copy)(void* Dest, const void* Src) = nullptr;
    bool (*Identical)(const void* A, const void* B) = nullptr;
    uint32 (*GetTypeHash)(const void* Src) = nullptr;
    // Reads a natively serialized value (UE's WithSerializer); null for a struct the cook writes as properties. False
    // when the struct's properties follow, as UE's Serialize returning false.
    bool (*Serialize)(FArchive& Ar, void* Data) = nullptr;
    bool bIsPlainOldData = false;
    bool bHasZeroConstructor = false;
};

// What a native struct supports, specialized next to the struct as in UE:
//   template <> struct TStructOpsTypeTraits<FFoo> : TStructOpsTypeTraitsBase2<FFoo> { enum { WithSerializer = true }; };
template <class CPPSTRUCT>
struct TStructOpsTypeTraitsBase2 {
    enum {
        WithZeroConstructor = false,       // zeroed memory is the default value; no constructor call
        WithNoDestructor = false,          // the destructor need not run
        WithCopy = !std::is_trivially_copyable_v<CPPSTRUCT>,
        WithIdenticalViaEquality = false,  // operator== compares
        WithIdentical = false,             // bool Identical(const CPPSTRUCT* Other, uint32 PortFlags) const compares
        WithSerializer = false,            // Serialize(FArchive&) reads the cooked value
    };
};
template <class CPPSTRUCT>
struct TStructOpsTypeTraits : TStructOpsTypeTraitsBase2<CPPSTRUCT> {};

// The FCppStructOps of a C++ struct, from its traits (UE's UScriptStruct::TCppStructOps). UnrealHeaderTool registers it
// for each USTRUCT.
template <class CPPSTRUCT>
constexpr FCppStructOps TCppStructOps() {
    using TTraits = TStructOpsTypeTraits<CPPSTRUCT>;
    FCppStructOps Ops;
    Ops.Size = sizeof(CPPSTRUCT);
    Ops.Alignment = alignof(CPPSTRUCT);
    Ops.bHasZeroConstructor = TTraits::WithZeroConstructor;
    Ops.bIsPlainOldData = std::is_trivially_copyable_v<CPPSTRUCT> && std::is_trivially_destructible_v<CPPSTRUCT>;
    if constexpr (!TTraits::WithZeroConstructor) Ops.Construct = [](void* Dest) { new (Dest) CPPSTRUCT(); };
    if constexpr (!TTraits::WithNoDestructor && !std::is_trivially_destructible_v<CPPSTRUCT>)
        Ops.Destruct = [](void* Dest) { static_cast<CPPSTRUCT*>(Dest)->~CPPSTRUCT(); };
    if constexpr (TTraits::WithCopy)
        Ops.Copy = [](void* Dest, const void* Src) { *static_cast<CPPSTRUCT*>(Dest) = *static_cast<const CPPSTRUCT*>(Src); };
    if constexpr (TTraits::WithIdentical)
        Ops.Identical = [](const void* A, const void* B) { return static_cast<const CPPSTRUCT*>(A)->Identical(static_cast<const CPPSTRUCT*>(B), 0u); };
    else if constexpr (TTraits::WithIdenticalViaEquality)
        Ops.Identical = [](const void* A, const void* B) { return *static_cast<const CPPSTRUCT*>(A) == *static_cast<const CPPSTRUCT*>(B); };
    if constexpr (requires(const CPPSTRUCT& Value) { GetTypeHash(Value); })
        Ops.GetTypeHash = [](const void* Src) { return static_cast<uint32>(GetTypeHash(*static_cast<const CPPSTRUCT*>(Src))); };
    if constexpr (TTraits::WithSerializer) Ops.Serialize = [](FArchive& Ar, void* Data) { return static_cast<CPPSTRUCT*>(Data)->Serialize(Ar); };
    return Ops;
}

class UScriptStruct : public UStruct {
    DECLARE_CLASS(UScriptStruct, UStruct, CASTCLASS_UScriptStruct)
    explicit UScriptStruct(const FObjectInitializer& ObjectInitializer);
public:
    EStructFlags StructFlags = STRUCT_NoFlags;
    const FCppStructOps* CppStructOps = nullptr;

    const FCppStructOps* GetCppStructOps() const { return CppStructOps; }
    void Link() override;
    // The size, alignment and struct flags: from the C++ operations, else from the properties.
    void PrepareCppStructOps();
    void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;
    void DestroyStruct(void* Dest, int32 ArrayDim = 1) const override;
    void CopyScriptStruct(void* Dest, const void* Src, int32 ArrayDim = 1) const;
    bool CompareScriptStruct(const void* A, const void* B, uint32 PortFlags = 0) const;
    void ClearScriptStruct(void* Dest, int32 ArrayDim = 1) const;
    uint32 GetStructTypeHash(const void* Src) const;
    // Reads a cooked struct value: natively serialized, or as properties.
    void SerializeItem(FArchive& Ar, void* Value) const;
    void Serialize(FArchive& Ar) override;
};

class UFunction : public UStruct {
    DECLARE_CLASS(UFunction, UStruct, CASTCLASS_UFunction)
    explicit UFunction(const FObjectInitializer& ObjectInitializer);
public:
    EFunctionFlags FunctionFlags = FUNC_None;
    uint8 NumParms = 0;
    uint16 ParmsSize = 0;
    uint16 ReturnValueOffset = 0xffff;
    uint16 RPCId = 0;
    uint16 RPCResponseId = 0;
    FProperty* FirstPropertyToInit = nullptr;
    // An event's stub calls straight into the ubergraph at this offset (UE's fast path).
    UFunction* EventGraphFunction = nullptr;
    int32 EventGraphCallOffset = 0;
    FNativeFuncPtr Func = nullptr;

    bool HasAnyFunctionFlags(uint32 Flags) const { return (FunctionFlags & Flags) != 0; }
    bool HasAllFunctionFlags(uint32 Flags) const { return (FunctionFlags & Flags) == Flags; }
    FNativeFuncPtr GetNativeFunc() const { return Func; }
    void SetNativeFunc(FNativeFuncPtr InFunc) { Func = InFunc; }
    FProperty* GetReturnProperty() const;
    UClass* GetOuterUClass() const;
    // Runs the function: its native thunk, or ProcessInternal for bytecode.
    void Invoke(UObject* Obj, FFrame& Stack, RESULT_DECL);
    // Links, then the derived members: NumParms, ParmsSize, ReturnValueOffset, FirstPropertyToInit.
    void Link() override;
    void Serialize(FArchive& Ar) override;
    // Turns the cooked bytecode into the memory format on first use. False when it cannot run.
    bool EnsureScriptLinked();
};

// UDelegateFunction: the signature of a delegate type.
class UDelegateFunction : public UFunction {
    DECLARE_CLASS(UDelegateFunction, UFunction, CASTCLASS_UDelegateFunction)
    explicit UDelegateFunction(const FObjectInitializer& ObjectInitializer);
};

class USparseDelegateFunction : public UDelegateFunction {
    DECLARE_CLASS(USparseDelegateFunction, UDelegateFunction, CASTCLASS_USparseDelegateFunction)
    explicit USparseDelegateFunction(const FObjectInitializer& ObjectInitializer);
};

struct FImplementedInterface {
    UClass* Class = nullptr;
    int32 PointerOffset = 0;
    bool bImplementedByK2 = false;
};

// The C++ constructor of a class: placement-constructs the most derived native C++ class at the initializer's object.
using FClassConstructorType = void (*)(const FObjectInitializer& ObjectInitializer);

class UClass : public UStruct {
    DECLARE_CLASS(UClass, UStruct, CASTCLASS_UClass)
    explicit UClass(const FObjectInitializer& ObjectInitializer);
public:
    FClassConstructorType ClassConstructor = nullptr;
    // The size of the C++ object that ClassConstructor builds; the reflected properties of the native class that have
    // no C++ member, and all Blueprint properties, come after it.
    int32 NativeSize = 0;
    EClassFlags ClassFlags = CLASS_None;
    uint64 ClassCastFlags = CASTCLASS_None;
    UClass* ClassWithin = nullptr;
    FName ClassConfigName;
    TArray<FImplementedInterface> Interfaces;
    UObject* ClassDefaultObject = nullptr;
    // The cooked default object of a class loaded from a package (an export of the same package).
    int32 ClassDefaultObjectIndex = 0;
    // The counter of MakeUniqueObjectName.
    int32 ClassUnique = 0;
    // A Blueprint class's sparse class data: values shared by all instances, read with its default object.
    UScriptStruct* SparseClassDataStruct = nullptr;
    void* SparseClassData = nullptr;

    UClass* GetSuperClass() const { return static_cast<UClass*>(GetSuperStruct()); }
    // The C++ constructor that builds an object of this class: the class's own, or the nearest native super's. A
    // Blueprint class cooked without a constructor for one of its intermediate super classes still builds as the
    // nearest C++ class; the cooked default object supplies the Blueprint defaults.
    // The size of the C++ object the constructor builds: the largest NativeSize in the chain. A Blueprint class
    // serialized while its super was not yet linked can carry a zero, so the nearest native class supplies it.
    int32 GetNativeSize() const {
        int32 Size = 0;
        for (const UClass* Class = this; Class != nullptr; Class = Class->GetSuperClass())
            if (Class->NativeSize > Size) Size = Class->NativeSize;
        return Size;
    }
    bool HasAnyClassFlags(uint32 Flags) const { return (ClassFlags & Flags) != 0; }
    // Inherits the super class's C++ part and links, now or once the super class has linked.
    void LinkAfterSuper();
    bool HasAllClassFlags(uint32 Flags) const { return (ClassFlags & Flags) == Flags; }
    bool HasAnyCastFlag(uint64 Flags) const { return (ClassCastFlags & Flags) != 0; }
    // The class default object, made when first asked for: cooked for a Blueprint class, constructed for a native one.
    UObject* GetDefaultObject(bool bCreateIfNeeded = true);
    template <class T>
    T* GetDefaultObject() {
        return static_cast<T*>(GetDefaultObject());
    }
    // The function of the name on this class, else its supers, else its interfaces.
    UFunction* FindFunctionByName(FName InName, bool bIncludeSuper = true) const;
    void AddFunctionToFunctionMap(UFunction* Function, FName FuncName);
    bool ImplementsInterface(const UClass* SomeInterface) const;
    // The address of an interface in an object: the object itself, as for every Blueprint-callable interface.
    void* GetInterfaceAddress(UObject* Object, const UClass* InterfaceClass) const;
    // The persistent frame of the ubergraph of a Blueprint class; null for other classes.
    virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const { return nullptr; }
    virtual void CreatePersistentUberGraphFrame(UObject* Obj, bool bCreateOnlyIfEmpty = false, bool bSkipSuperClass = false,
                                                UClass* OldClass = nullptr) const {}
    virtual void DestroyPersistentUberGraphFrame(UObject* Obj, bool bSkipSuperClass = false) const {}
    // Called on a new object of this class after its properties are initialized (Blueprint classes make their
    // persistent frame here).
    virtual void PostInitInstance(UObject* Obj) const {}
    // Copies the defaults of the properties that the PostConstructLink chain leaves out: a Blueprint class's values of
    // its native supers' properties (UE's InitPropertiesFromCustomList).
    virtual void InitPropertiesFromCustomList(uint8* DataPtr, const uint8* DefaultDataPtr) {}
    void Link() override;
    void Serialize(FArchive& Ar) override;
    // Reads a cooked class default object: its properties (laid out as this class, defaults from the super's default
    // object), then the sparse class data (UE's SerializeDefaultObject). A default object reads no native data.
    virtual void SerializeDefaultObject(UObject* Object, FArchive& Ar);
    const void* GetSparseClassData() const { return SparseClassData; }
    // The struct of the sparse class data: this class's own, else its super's (UE sets it in SetSuperStruct).
    UScriptStruct* GetSparseClassDataStruct() const;
    ~UClass() override;

protected:
    // UE's InternalCreateDefaultObjectWrapper: builds the default object.
    virtual UObject* CreateDefaultObject();

    TArray<std::pair<FName, UFunction*>> FuncMap;
    std::recursive_mutex DefaultObjectMutex;
};

enum class ECppForm : uint8 { Regular, Namespaced, EnumClass };

class UEnum : public UField {
    DECLARE_CLASS(UEnum, UField, CASTCLASS_UEnum)
    explicit UEnum(const FObjectInitializer& ObjectInitializer);
public:
    // The entries with their full names ("EMyEnum::Value" for a namespaced enum) and values.
    TArray<std::pair<FName, int64>> Names;
    ECppForm CppForm = ECppForm::Regular;

    int32 NumEnums() const { return Names.Num(); }
    FName GetNameByIndex(int32 Index) const;
    int64 GetValueByIndex(int32 Index) const;
    int32 GetIndexByValue(int64 Value) const;
    int32 GetIndexByName(FName InName) const;
    int64 GetValueByName(FName InName) const;
    FName GetNameByValue(int64 Value) const;
    // The name without the "EnumName::" prefix.
    FString GetNameStringByIndex(int32 Index) const;
    FString GetNameStringByValue(int64 Value) const;
    // The authored display name; the runtime has no metadata, so it is the short name, as UE without editor data.
    FText GetDisplayNameTextByIndex(int32 Index) const;
    int64 GetMaxEnumValue() const;
    bool IsValidEnumValue(int64 Value) const { return GetIndexByValue(Value) != INDEX_NONE; }
    void Serialize(FArchive& Ar) override;
};

// UPackage: the outermost object of every package ("/Game/A/B", "/Script/Engine").
class UPackage : public UObject {
    DECLARE_CLASS(UPackage, UObject, CASTCLASS_UPackage)
    explicit UPackage(const FObjectInitializer& ObjectInitializer);
};

// UInterface: the base class of interface classes.
class UInterface : public UObject {
    DECLARE_CLASS(UInterface, UObject, 0)
    explicit UInterface(const FObjectInitializer& ObjectInitializer);
};

}

#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"
