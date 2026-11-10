#pragma once

#include <atomic>
#include <span>

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

namespace ue5parse {

class FileProvider;

}

namespace ue5parse::runtime {

class FObjectInstancingGraph;

class FObjectInitializer {
public:
    FObjectInitializer(UObject* InObj, UObject* InObjectArchetype, bool bInCopyTransientsFromClassDefaults = false, bool bInShouldInitializeProps = true,
                       FObjectInstancingGraph* InInstanceGraph = nullptr);
    ~FObjectInitializer();
    FObjectInitializer(const FObjectInitializer&) = delete;
    FObjectInitializer& operator=(const FObjectInitializer&) = delete;

    UObject* GetObj() const { return Obj; }
    UClass* GetClass() const;
    UObject* GetArchetype() const { return ObjectArchetype; }
    static FObjectInitializer& Get();

    UObject* CreateDefaultSubobject(UObject* Outer, FName SubobjectFName, UClass* ReturnType, UClass* ClassToCreateByDefault, bool bIsRequired = true,
                                    bool bIsTransient = false) const;
    template <class T>
    T* CreateDefaultSubobject(FName SubobjectName, bool bTransient = false) const {
        return static_cast<T*>(CreateDefaultSubobject(Obj, SubobjectName, T::StaticClass(), T::StaticClass(), true, bTransient));
    }
    template <class T>
    T* CreateOptionalDefaultSubobject(FName SubobjectName, bool bTransient = false) const {
        return static_cast<T*>(CreateDefaultSubobject(Obj, SubobjectName, T::StaticClass(), T::StaticClass(), false, bTransient));
    }
    UObject* CreateDefaultSubobject(FName SubobjectName, const char* ClassName, bool bIsRequired = true, bool bIsTransient = false) const;
    template <class T>
    const FObjectInitializer& SetDefaultSubobjectClass(FName SubobjectName) const {
        SubobjectOverrides.Add(FSubobjectOverride{SubobjectName, T::StaticClass()});
        return *this;
    }

    static void InitProperties(UObject* Obj, UClass* DefaultsClass, UObject* DefaultData, bool bCopyTransientsFromClassDefaults);

    struct FSubobjectInit {
        UObject* Subobject;
        UObject* Template;
    };
    struct FSubobjectOverride {
        FName Name;
        UClass* Class;
    };

private:
    friend class UObjectBase;
    void PostConstructInit();

    UObject* Obj;
    UObject* ObjectArchetype;
    bool bCopyTransientsFromClassDefaults;
    bool bShouldInitializePropsFromArchetype;
    FObjectInstancingGraph* InstanceGraph;
    mutable TArray<FSubobjectInit> SubobjectInits;
    mutable TArray<FSubobjectOverride> SubobjectOverrides;
    FObjectInitializer* PreviousInitializer;
};

class FObjectInstancingGraph {
public:
    explicit FObjectInstancingGraph(UObject* InSourceRoot = nullptr, UObject* InDestinationRoot = nullptr)
        : SourceRoot(InSourceRoot), DestinationRoot(InDestinationRoot) {}

    void AddNewObject(UObject* ObjectInstance, UObject* InArchetype);
    UObject* GetDestinationObject(UObject* SourceObject) const;
    UObject* InstancePropertyValue(UObject* ComponentTemplate, UObject* Owner, bool bIsTransient, bool bCausesInstancing);

    UObject* SourceRoot;
    UObject* DestinationRoot;

private:
    TArray<std::pair<UObject*, UObject*>> SourceToDestinationMap;
    bool bEnableSubobjectInstancing = true;
};

void InstanceSubobjectTemplates(const UStruct* Struct, void* Data, const void* DefaultData, const UStruct* DefaultStruct, UObject* Owner,
                                FObjectInstancingGraph* InstanceGraph);

struct FStaticConstructObjectParameters {
    const UClass* Class = nullptr;
    UObject* Outer = nullptr;
    FName Name;
    EObjectFlags SetFlags = RF_NoFlags;
    UObject* Template = nullptr;
    bool bCopyTransientsFromClassDefaults = false;
    FObjectInstancingGraph* InstanceGraph = nullptr;
};

UObject* StaticAllocateObject(const UClass* Class, UObject* Outer, FName Name, EObjectFlags SetFlags);
UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params);

template <class T>
T* NewObject(UObject* Outer, const UClass* Class = nullptr, FName Name = FName(), EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr) {
    FStaticConstructObjectParameters Params;
    Params.Class = Class ? Class : T::StaticClass();
    Params.Outer = Outer;
    Params.Name = Name;
    Params.SetFlags = Flags;
    Params.Template = Template;
    return static_cast<T*>(StaticConstructObject_Internal(Params));
}

FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName = FName());
UObject* StaticFindObjectFast(const UClass* Class, UObject* Outer, FName Name, bool bExactClass = false);
UObject* StaticFindObject(const UClass* Class, UObject* Outer, const FString& Path, bool bExactClass = false);
UObject* StaticLoadObject(const UClass* Class, UObject* Outer, const FString& Path);
template <class T>
T* LoadObject(UObject* Outer, const FString& Path) {
    return static_cast<T*>(StaticLoadObject(T::StaticClass(), Outer, Path));
}
UClass* StaticLoadClass(const UClass* BaseClass, UObject* Outer, const FString& Path);
UPackage* LoadPackage(const FString& PackageName);
UPackage* FindPackage(const FString& PackageName);
UPackage* CreatePackage(const FString& PackageName);
UPackage* GetTransientPackage();

void DestroyObject(UObjectBase* Object);
void RenameObject(UObjectBase* Object, FName NewName, UObject* NewOuter);
void CollectGarbage();

struct FNativeMember {
    const char* Name;
    int32 Offset;
    int32 Size;
    uint64 PropertyFlags = 0;
    const char* RepNotifyFunc = nullptr;
};

struct FNativeParam {
    const char* Name;
    const char* Type;
    uint64 Flags;
};

struct FNativeFunction {
    const char* Name;
    FNativeFuncPtr Func;
    uint32 Flags = 0;
    std::span<const FNativeParam> Params = {};
};

struct FNativeClassInfo {
    const char* Package;
    const char* Name;
    const char* SuperName;
    int32 Size;
    int32 Alignment;
    FClassConstructorType Constructor;
    uint32 ClassFlags = 0;
    uint64 CastFlags = 0;
    std::span<const FNativeMember> Members = {};
    std::span<const FNativeFunction> Functions = {};
};

struct FNativeStructInfo {
    const char* Package;
    const char* Name;
    const FCppStructOps* Ops;
    std::span<const FNativeMember> Members = {};
};

struct FNativeDelegateFunctionInfo {
    const char* Package;
    const char* Name;
    uint32 Flags = 0;
    std::span<const FNativeParam> Params = {};
};

struct FNativeFunctionTable {
    const char* ClassName;
    std::span<const FNativeFunction> Functions;
};

template <class T>
void InternalConstructor(const FObjectInitializer& X) {
    new (static_cast<void*>(X.GetObj())) T(X);
}

#define IMPLEMENT_CLASS(TClass, TName)                                                                                                                         \
    UClass* TClass::StaticClass() {                                                                                                                            \
        static constinit std::atomic<UClass*> Class = nullptr;                                                                                                 \
        if (UClass* Found = Class.load(std::memory_order_acquire))                                                                                             \
            return Found;                                                                                                                                      \
        return GetNativeClass(Class, TName);                                                                                                                   \
    }

#define NATIVE_MEMBER(TClass, TMember)                                                                                                                         \
    FNativeMember {                                                                                                                                            \
        #TMember, static_cast<int32>(offsetof(TClass, TMember)), static_cast<int32>(sizeof(TClass::TMember))                                                   \
    }
#define NATIVE_CLASS_INFO(TClass, TPackage, TName, TSuperName, ...)                                                                                            \
    FNativeClassInfo {                                                                                                                                         \
        TPackage, TName, TSuperName, sizeof(TClass), alignof(TClass), &InternalConstructor<TClass>, __VA_ARGS__                                                \
    }

void RegisterNativeClasses(std::span<const FNativeClassInfo> Classes);
void RegisterNativeStructs(std::span<const FNativeStructInfo> Structs);
void RegisterNativeFunctions(std::span<const FNativeFunctionTable> Tables);
void RegisterNativeDelegateFunctions(std::span<const FNativeDelegateFunctionInfo> Delegates);

struct FRegisterCompiledInInfo {
    FRegisterCompiledInInfo(std::span<const FNativeClassInfo> Classes, std::span<const FNativeStructInfo> Structs,
                            std::span<const FNativeDelegateFunctionInfo> Delegates = {}) {
        RegisterNativeClasses(Classes);
        RegisterNativeStructs(Structs);
        RegisterNativeDelegateFunctions(Delegates);
    }
};
class FRuntime {
public:
    explicit FRuntime(const FileProvider& Provider);
    ~FRuntime();
    FRuntime(const FRuntime&) = delete;
    FRuntime& operator=(const FRuntime&) = delete;
};

const FileProvider& GetRuntimeProvider();

UClass* FindNativeClass(FName Name);
UClass* GetNativeClass(const char* Name);
//! No function-local static: its initialization guard would deadlock against a thread that holds the loader mutex.
UClass* GetNativeClass(std::atomic<UClass*>& Cache, const char* Name);
UScriptStruct* FindNativeStruct(std::atomic<UScriptStruct*>& Cache, const char* Name);
UEnum* FindNativeEnum(std::atomic<UEnum*>& Cache, const char* Name);
UScriptStruct* FindNativeStruct(FName Name);
UEnum* FindNativeEnum(FName Name);
UFunction* FindNativeDelegateFunction(FName Name);

}
