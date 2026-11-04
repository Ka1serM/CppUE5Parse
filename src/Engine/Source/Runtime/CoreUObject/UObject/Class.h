#pragma once

#include <mutex>
#include <new>
#include <type_traits>

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

class FArchive;

class UField : public UObject {
    DECLARE_CLASS(UField, UObject, CASTCLASS_UField)
    explicit UField(const FObjectInitializer& ObjectInitializer);
public:
    UField* Next = nullptr;
};

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
    FProperty* FindPropertyByName(FName InName) const;
    void AddCppProperty(FProperty* Property);

    virtual void Link();
    void BuildPropertyChains();
    virtual void InitializeStruct(void* Dest, int32 ArrayDim = 1) const;
    virtual void DestroyStruct(void* Dest, int32 ArrayDim = 1) const;
    void SerializeTaggedProperties(FArchive& Ar, uint8* Data) const;
    void Serialize(FArchive& Ar) override;

    struct FUnversionedSlot {
        FProperty* Property = nullptr;
        int32 ArrayIndex = 0;
    };
    TArray<FUnversionedSlot> OwnUnversionedSlots;
    TArray<FUnversionedSlot> UnversionedSchema;
    void BuildUnversionedSchema();

    UStruct* SuperStruct = nullptr;
    UField* Children = nullptr;
    FField* ChildProperties = nullptr;
    int32 PropertiesSize = 0;
    int32 MinAlignment = 1;
    TArray<uint8> Script;
    TArray<FName> ScriptNames;
    TArray<uint8> SerializedScript;
    int32 BytecodeBufferSize = 0;
    FProperty* PropertyLink = nullptr;
    FProperty* RefLink = nullptr;
    FProperty* DestructorLink = nullptr;
    FProperty* PostConstructLink = nullptr;
    TArray<FProperty*> PropertiesByName;

protected:
    friend class FScriptLinker;
    bool bLinked = false;
    bool bScriptLinked = false;
};

struct FCppStructOps {
    int32 Size = 0;
    int32 Alignment = 1;
    void (*Construct)(void* Dest) = nullptr;
    void (*Destruct)(void* Dest) = nullptr;
    void (*Copy)(void* Dest, const void* Src) = nullptr;
    bool (*Identical)(const void* A, const void* B) = nullptr;
    uint32 (*GetTypeHash)(const void* Src) = nullptr;
    bool (*Serialize)(FArchive& Ar, void* Data) = nullptr;
    bool bIsPlainOldData = false;
    bool bHasZeroConstructor = false;
};

template <class CPPSTRUCT>
struct TStructOpsTypeTraitsBase2 {
    enum {
        WithZeroConstructor = false,
        WithNoDestructor = false,
        WithCopy = true,
        WithIdenticalViaEquality = false,
        WithIdentical = false,
        WithSerializer = false,
    };
};
template <class CPPSTRUCT>
struct TStructOpsTypeTraits : TStructOpsTypeTraitsBase2<CPPSTRUCT> {};

template <class CPPSTRUCT>
constexpr FCppStructOps TCppStructOps() {
    using TTraits = TStructOpsTypeTraits<CPPSTRUCT>;
    FCppStructOps Ops;
    Ops.Size = sizeof(CPPSTRUCT);
    Ops.Alignment = alignof(CPPSTRUCT);
    Ops.bHasZeroConstructor = TTraits::WithZeroConstructor;
    Ops.bIsPlainOldData = TTraits::WithCopy && std::is_trivially_copyable_v<CPPSTRUCT> && std::is_trivially_destructible_v<CPPSTRUCT>;
    if constexpr (!TTraits::WithZeroConstructor) Ops.Construct = [](void* Dest) { new (Dest) CPPSTRUCT(); };
    if constexpr (!TTraits::WithNoDestructor && !std::is_trivially_destructible_v<CPPSTRUCT>)
        Ops.Destruct = [](void* Dest) { static_cast<CPPSTRUCT*>(Dest)->~CPPSTRUCT(); };
    if constexpr (TTraits::WithCopy && !std::is_trivially_copyable_v<CPPSTRUCT>)
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
    void PrepareCppStructOps();
    void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;
    void DestroyStruct(void* Dest, int32 ArrayDim = 1) const override;
    void CopyScriptStruct(void* Dest, const void* Src, int32 ArrayDim = 1) const;
    bool CompareScriptStruct(const void* A, const void* B, uint32 PortFlags = 0) const;
    void ClearScriptStruct(void* Dest, int32 ArrayDim = 1) const;
    uint32 GetStructTypeHash(const void* Src) const;
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
    UFunction* EventGraphFunction = nullptr;
    int32 EventGraphCallOffset = 0;
    FNativeFuncPtr Func = nullptr;

    bool HasAnyFunctionFlags(uint32 Flags) const { return (FunctionFlags & Flags) != 0; }
    FNativeFuncPtr GetNativeFunc() const { return Func; }
    FProperty* GetReturnProperty() const;
    UClass* GetOuterUClass() const;
    void Invoke(UObject* Obj, FFrame& Stack, RESULT_DECL);
    void Link() override;
    void Serialize(FArchive& Ar) override;
    bool EnsureScriptLinked();
};

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

using FClassConstructorType = void (*)(const FObjectInitializer& ObjectInitializer);

class UClass : public UStruct {
    DECLARE_CLASS(UClass, UStruct, CASTCLASS_UClass)
    explicit UClass(const FObjectInitializer& ObjectInitializer);
public:
    FClassConstructorType ClassConstructor = nullptr;
    int32 NativeSize = 0;
    EClassFlags ClassFlags = CLASS_None;
    uint64 ClassCastFlags = CASTCLASS_None;
    UClass* ClassWithin = nullptr;
    FName ClassConfigName;
    TArray<FImplementedInterface> Interfaces;
    UObject* ClassDefaultObject = nullptr;
    int32 ClassDefaultObjectIndex = 0;
    int32 ClassUnique = 0;
    UScriptStruct* SparseClassDataStruct = nullptr;
    void* SparseClassData = nullptr;

    UClass* GetSuperClass() const { return static_cast<UClass*>(GetSuperStruct()); }
    int32 GetNativeSize() const {
        int32 Size = 0;
        for (const UClass* Class = this; Class != nullptr; Class = Class->GetSuperClass())
            if (Class->NativeSize > Size) Size = Class->NativeSize;
        return Size;
    }
    bool HasAnyClassFlags(uint32 Flags) const { return (ClassFlags & Flags) != 0; }
    void LinkAfterSuper();
    bool HasAnyCastFlag(uint64 Flags) const { return (ClassCastFlags & Flags) != 0; }
    UObject* GetDefaultObject(bool bCreateIfNeeded = true);
    template <class T>
    T* GetDefaultObject() {
        return static_cast<T*>(GetDefaultObject());
    }
    UFunction* FindFunctionByName(FName InName, bool bIncludeSuper = true) const;
    void AddFunctionToFunctionMap(UFunction* Function, FName FuncName);
    bool ImplementsInterface(const UClass* SomeInterface) const;
    void* GetInterfaceAddress(UObject* Object, const UClass* InterfaceClass) const;
    virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const { return nullptr; }
    virtual void CreatePersistentUberGraphFrame(UObject* Obj, bool bCreateOnlyIfEmpty = false, bool bSkipSuperClass = false,
                                                UClass* OldClass = nullptr) const {}
    virtual void DestroyPersistentUberGraphFrame(UObject* Obj, bool bSkipSuperClass = false) const {}
    virtual void PostInitInstance(UObject* Obj) const {}
    virtual void InitPropertiesFromCustomList(uint8* DataPtr, const uint8* DefaultDataPtr) {}
    void Link() override;
    void Serialize(FArchive& Ar) override;
    virtual void SerializeDefaultObject(UObject* Object, FArchive& Ar);
    const void* GetSparseClassData() const { return SparseClassData; }
    UScriptStruct* GetSparseClassDataStruct() const;
    ~UClass() override;

protected:
    virtual UObject* CreateDefaultObject();

    TArray<std::pair<FName, UFunction*>> FuncMap;
    std::recursive_mutex DefaultObjectMutex;
};

enum class ECppForm : uint8 { Regular, Namespaced, EnumClass };

class UEnum : public UField {
    DECLARE_CLASS(UEnum, UField, CASTCLASS_UEnum)
    explicit UEnum(const FObjectInitializer& ObjectInitializer);
public:
    TArray<std::pair<FName, int64>> Names;
    ECppForm CppForm = ECppForm::Regular;

    FName GetNameByIndex(int32 Index) const;
    int32 GetIndexByValue(int64 Value) const;
    int32 GetIndexByName(FName InName) const;
    int64 GetValueByName(FName InName) const;
    FName GetNameByValue(int64 Value) const;
    FString GetNameStringByIndex(int32 Index) const;
    FString GetNameStringByValue(int64 Value) const;
    FText GetDisplayNameTextByIndex(int32 Index) const;
    int64 GetMaxEnumValue() const;
    void Serialize(FArchive& Ar) override;
};

class UPackage : public UObject {
    DECLARE_CLASS(UPackage, UObject, CASTCLASS_UPackage)
    explicit UPackage(const FObjectInitializer& ObjectInitializer);
};

class UInterface : public UObject {
    DECLARE_CLASS(UInterface, UObject, 0)
    explicit UInterface(const FObjectInitializer& ObjectInitializer);
};

}

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"
