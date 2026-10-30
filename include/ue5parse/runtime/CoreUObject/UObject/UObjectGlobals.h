#pragma once

#include <span>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

namespace ue5parse {
class FileProvider;
}

namespace ue5parse::runtime {

class FObjectInstancingGraph;

// FObjectInitializer: what a new object is built from. Its constructor makes it the current initializer (UObjectBase's
// constructor reads the object's identity from it), and its destructor finishes the object after the C++ constructors
// ran: copies the archetype's property values, instances subobjects, calls PostInitProperties (UE's PostConstructInit).
class FObjectInitializer {
public:
    FObjectInitializer(UObject* InObj, UObject* InObjectArchetype, bool bInCopyTransientsFromClassDefaults = false,
                       bool bInShouldInitializeProps = true, FObjectInstancingGraph* InInstanceGraph = nullptr);
    ~FObjectInitializer();
    FObjectInitializer(const FObjectInitializer&) = delete;
    FObjectInitializer& operator=(const FObjectInitializer&) = delete;

    UObject* GetObj() const { return Obj; }
    UClass* GetClass() const;
    UObject* GetArchetype() const { return ObjectArchetype; }
    // The initializer of the object under construction on this thread.
    static FObjectInitializer& Get();

    // Creates a subobject in a constructor, with the archetype's subobject of the same name as its template
    // (UE's CreateDefaultSubobject). A class override of the archetype's subobject wins over ClassToCreateByDefault.
    UObject* CreateDefaultSubobject(UObject* Outer, FName SubobjectFName, UClass* ReturnType, UClass* ClassToCreateByDefault,
                                    bool bIsRequired = true, bool bIsTransient = false) const;
    template <class T>
    T* CreateDefaultSubobject(FName SubobjectName, bool bTransient = false) const {
        return static_cast<T*>(CreateDefaultSubobject(Obj, SubobjectName, T::StaticClass(), T::StaticClass(), true, bTransient));
    }
    template <class T>
    T* CreateOptionalDefaultSubobject(FName SubobjectName, bool bTransient = false) const {
        return static_cast<T*>(CreateDefaultSubobject(Obj, SubobjectName, T::StaticClass(), T::StaticClass(), false, bTransient));
    }
    // A default subobject of a class that has only its mappings here, by the class's name; null when the mappings do
    // not know it either.
    UObject* CreateDefaultSubobject(FName SubobjectName, const char* ClassName, bool bIsRequired = true, bool bIsTransient = false) const;
    // A subclass constructor's choice for a default subobject of its supers: another class, or none for an optional one.
    template <class T>
    const FObjectInitializer& SetDefaultSubobjectClass(FName SubobjectName) const {
        SubobjectOverrides.Add(FSubobjectOverride{SubobjectName, T::StaticClass()});
        return *this;
    }
    const FObjectInitializer& DoNotCreateDefaultSubobject(FName SubobjectName) const {
        SubobjectOverrides.Add(FSubobjectOverride{SubobjectName, nullptr});
        return *this;
    }

    // Copies the property values of DefaultData into Obj, as UE: a native object built from its class default
    // object only needs the values C++ does not set; any other object takes every property.
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

// The template-to-instance map of one construction: a reference in the new object to a subobject of its archetype
// becomes a reference to the new object's own copy (UE's FObjectInstancingGraph).
class FObjectInstancingGraph {
public:
    explicit FObjectInstancingGraph(UObject* InSourceRoot = nullptr, UObject* InDestinationRoot = nullptr)
        : SourceRoot(InSourceRoot), DestinationRoot(InDestinationRoot) {}

    void AddNewObject(UObject* ObjectInstance, UObject* InArchetype);
    UObject* GetDestinationObject(UObject* SourceObject) const;
    // The instance of a referenced subobject template: found, or made when the property is an instanced reference.
    UObject* InstancePropertyValue(UObject* ComponentTemplate, UObject* Owner, bool bIsTransient, bool bCausesInstancing);
    void EnableSubobjectInstancing(bool bEnabled) { bEnableSubobjectInstancing = bEnabled; }
    bool IsSubobjectInstancingEnabled() const { return bEnableSubobjectInstancing; }

    UObject* SourceRoot;
    UObject* DestinationRoot;

private:
    TArray<std::pair<UObject*, UObject*>> SourceToDestinationMap;
    bool bEnableSubobjectInstancing = true;
};

// Walks the object references of Data (laid out as Struct) and replaces references to subobjects of the archetype
// with their instances (UE's UStruct::InstanceSubobjectTemplates).
void InstanceSubobjectTemplates(const UStruct* Struct, void* Data, const void* DefaultData, const UStruct* DefaultStruct,
                                UObject* Owner, FObjectInstancingGraph* InstanceGraph);

struct FStaticConstructObjectParameters {
    const UClass* Class = nullptr;
    UObject* Outer = nullptr;
    FName Name;
    EObjectFlags SetFlags = RF_NoFlags;
    UObject* Template = nullptr;
    bool bCopyTransientsFromClassDefaults = false;
    FObjectInstancingGraph* InstanceGraph = nullptr;
};

// Allocates and registers an object's memory (zeroed) without constructing it.
UObject* StaticAllocateObject(const UClass* Class, UObject* Outer, FName Name, EObjectFlags SetFlags);
UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params);

// NewObject<T>(Outer, Class, Name, Flags, Template): a new object; a None name gets a unique one.
template <class T>
T* NewObject(UObject* Outer, const UClass* Class = nullptr, FName Name = FName(), EObjectFlags Flags = RF_NoFlags,
             UObject* Template = nullptr) {
    FStaticConstructObjectParameters Params;
    Params.Class = Class ? Class : T::StaticClass();
    Params.Outer = Outer;
    Params.Name = Name;
    Params.SetFlags = Flags;
    Params.Template = Template;
    return static_cast<T*>(StaticConstructObject_Internal(Params));
}

FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName = FName());
// The object of a name in an outer, or null.
UObject* StaticFindObjectFast(const UClass* Class, UObject* Outer, FName Name, bool bExactClass = false);
// An object by path ("/Game/A/B.B", "/Game/A/B.B:Sub", "/Script/Engine.Actor"), when it is loaded.
UObject* StaticFindObject(const UClass* Class, UObject* Outer, const FString& Path, bool bExactClass = false);
template <class T>
T* FindObject(UObject* Outer, const FString& Path, bool bExactClass = false) {
    return static_cast<T*>(StaticFindObject(T::StaticClass(), Outer, Path, bExactClass));
}
// Loads the package of a path when needed and finds the object (UE's StaticLoadObject). Null when it does not exist
// or is not of the class.
UObject* StaticLoadObject(const UClass* Class, UObject* Outer, const FString& Path);
template <class T>
T* LoadObject(UObject* Outer, const FString& Path) {
    return static_cast<T*>(StaticLoadObject(T::StaticClass(), Outer, Path));
}
UClass* StaticLoadClass(const UClass* BaseClass, UObject* Outer, const FString& Path);
UPackage* LoadPackage(const FString& PackageName);
UPackage* FindPackage(const FString& PackageName);
// The package of a name, made when it does not exist (UE's CreatePackage).
UPackage* CreatePackage(const FString& PackageName);
UPackage* GetTransientPackage();

// Destroys an object now: BeginDestroy, FinishDestroy, its non-native properties, its C++ destructor, its memory.
// References to it in other objects are not cleared: use CollectGarbage for that.
void DestroyObject(UObjectBase* Object);
void RenameObject(UObjectBase* Object, FName NewName, UObject* NewOuter);
// Destroys every object marked as garbage (and objects whose outer is garbage), after clearing the references to them
// in all other objects' object properties, as UE's garbage collector does for garbage objects.
void CollectGarbage();

// A C++ member of a native class or struct that a reflected property maps to, with the replication that its UPROPERTY
// declares (Replicated: CPF_Net; ReplicatedUsing = Func: also CPF_RepNotify and the RepNotify function).
struct FNativeMember {
    const char* Name;
    int32 Offset;
    int32 Size;
    uint64 PropertyFlags = 0;
    const char* RepNotifyFunc = nullptr;
};

// A parameter of a native function, as UnrealHeaderTool writes it (UE's FPropertyParams of a UFunction): its name,
// its type and its property flags. The type is "bool", "int32", "float", "Name", "Str", "Text", ..., "Struct:Vector",
// "Object:Actor", "Class:Actor", "SoftObject:Texture2D", "SoftClass:Actor", "WeakObject:Actor", "Interface:Foo",
// "Enum:ETeleportType:uint8", "Byte:EMyEnum", "Array<Struct:Vector>", "Set<Name>", "Map<Name,int32>".
struct FNativeParam {
    const char* Name;
    const char* Type;
    uint64 Flags;
};

// A native function: its name on its class, its thunk (null for an event that only Blueprints implement), its flags
// and its parameters, so that ProcessEvent and delegates can call it as UE does. A function with a thunk is native.
struct FNativeFunction {
    const char* Name;
    FNativeFuncPtr Func;
    uint32 Flags = 0;
    std::span<const FNativeParam> Params = {};
};

// What UE's generated code registers for a native class: its C++ size and constructor, its members, its functions.
struct FNativeClassInfo {
    const char* Package;   // "/Script/Engine"
    const char* Name;      // "Actor"
    const char* SuperName; // "Object"; null for UObject
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
    const char* Name;   // "Vector"
    const FCppStructOps* Ops;
    std::span<const FNativeMember> Members = {};
};

// A native delegate signature: the UDelegateFunction that a cooked delegate property's signature points at.
struct FNativeDelegateFunctionInfo {
    const char* Package;
    const char* Name;   // "OnMontageBlendingOutStartedMCDelegate__DelegateSignature"
    uint32 Flags = 0;
    std::span<const FNativeParam> Params = {};
};

// Functions of a native class that the class itself does not register (another module adds them by class name).
struct FNativeFunctionTable {
    const char* ClassName;
    std::span<const FNativeFunction> Functions;
};

template <class T>
void InternalConstructor(const FObjectInitializer& X) {
    new (static_cast<void*>(X.GetObj())) T(X);
}

#define IMPLEMENT_CLASS(TClass, TName)                                                                                 \
    UClass* TClass::StaticClass() {                                                                                    \
        static UClass* const Class = GetNativeClass(TName);                                                            \
        return Class;                                                                                                  \
    }

#define NATIVE_MEMBER(TClass, TMember)                                                                                 \
    FNativeMember{#TMember, static_cast<int32>(offsetof(TClass, TMember)), static_cast<int32>(sizeof(TClass::TMember))}
#define NATIVE_CLASS_INFO(TClass, TPackage, TName, TSuperName, ...)                                                    \
    FNativeClassInfo{TPackage, TName, TSuperName, sizeof(TClass), alignof(TClass), &InternalConstructor<TClass>, __VA_ARGS__}

// The registry of the native C++ parts. Modules add theirs once, before the first FRuntime starts.
void RegisterNativeClasses(std::span<const FNativeClassInfo> Classes);
void RegisterNativeStructs(std::span<const FNativeStructInfo> Structs);
void RegisterNativeFunctions(std::span<const FNativeFunctionTable> Tables);
void RegisterNativeDelegateFunctions(std::span<const FNativeDelegateFunctionInfo> Delegates);

// The registration of one generated file: a static object in each Foo.gen.cpp registers its classes and structs when
// the program starts (UE's FRegisterCompiledInInfo). Link the module whole-archive so that no .gen.cpp is dropped.
struct FRegisterCompiledInInfo {
    FRegisterCompiledInInfo(std::span<const FNativeClassInfo> Classes, std::span<const FNativeStructInfo> Structs,
                            std::span<const FNativeDelegateFunctionInfo> Delegates = {}) {
        RegisterNativeClasses(Classes);
        RegisterNativeStructs(Structs);
        RegisterNativeDelegateFunctions(Delegates);
    }
};
// The object system on a mounted provider (its mappings give the native classes' reflected properties). One runtime
// loads at a time: a new one replaces the one before it, and the objects of that one become invalid. The provider must
// outlive the runtime, and every object loaded from it is destroyed with it.
class FRuntime {
public:
    explicit FRuntime(const FileProvider& Provider);
    ~FRuntime();
    FRuntime(const FRuntime&) = delete;
    FRuntime& operator=(const FRuntime&) = delete;
};

const FileProvider& GetRuntimeProvider();

// A native class by name ("Actor"), built on first use from its registration and the mappings: the registered C++
// part of the nearest registered class of its chain, its reflected properties from the mappings. Null when neither
// knows the name.
UClass* FindNativeClass(FName Name);
// As FindNativeClass; a class the C++ code names must exist.
UClass* GetNativeClass(const char* Name);
UScriptStruct* FindNativeStruct(FName Name);
UEnum* FindNativeEnum(FName Name);
// A native delegate signature function registered by a module's generated code.
UFunction* FindNativeDelegateFunction(FName Name);

}
