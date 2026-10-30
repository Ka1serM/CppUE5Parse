#include <cstring>

#include <ankerl/unordered_dense.h>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectArray.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectHash.h"

namespace ue5parse::runtime {

namespace {

// The identity of the object StaticAllocateObject made last on this thread, which UObjectBase's constructor takes.
struct FPendingIdentity {
    UObjectBase* Object = nullptr;
    EObjectFlags Flags = RF_NoFlags;
    int32 InternalIndex = INDEX_NONE;
    UClass* Class = nullptr;
    FName Name;
    UObject* Outer = nullptr;
};

thread_local FPendingIdentity GPendingIdentity;
thread_local FObjectInitializer* GTopInitializer = nullptr;

struct FObjectKey {
    const UObject* Outer;
    FName Name;
    bool operator==(const FObjectKey& Other) const = default;
};

struct FObjectKeyHash {
    using is_avalanching = void;
    uint64 operator()(const FObjectKey& Key) const {
        return ankerl::unordered_dense::hash<uint64>{}(static_cast<uint64>(reinterpret_cast<uintptr_t>(Key.Outer)) ^ FNameHasher()(Key.Name));
    }
};

// The objects by outer and name, and the objects of each outer (UE's FUObjectHashTables).
struct FObjectHashTables {
    std::recursive_mutex Mutex;
    ankerl::unordered_dense::map<FObjectKey, UObject*, FObjectKeyHash> ByName;
    ankerl::unordered_dense::map<const UObject*, TArray<UObject*>> ByOuter;

    void Add(UObject* Object, UObject* Outer, FName Name) {
        std::lock_guard Lock(Mutex);
        ByName[FObjectKey{Outer, Name}] = Object;
        ByOuter[Outer].Add(Object);
    }
    void Remove(UObject* Object) {
        std::lock_guard Lock(Mutex);
        auto Found = ByName.find(FObjectKey{Object->GetOuter(), Object->GetFName()});
        if (Found != ByName.end() && Found->second == Object) ByName.erase(Found);
        auto Children = ByOuter.find(Object->GetOuter());
        if (Children != ByOuter.end()) Children->second.Remove(Object);
    }
};

FObjectHashTables& GetObjectHashTables() {
    static FObjectHashTables Tables;
    return Tables;
}

UPackage* GTransientPackage = nullptr;

}

std::recursive_mutex& GetLoaderMutex() {
    static std::recursive_mutex Mutex;
    return Mutex;
}

UObjectBase::UObjectBase()
    : ObjectFlags(GPendingIdentity.Flags), InternalIndex(GPendingIdentity.InternalIndex), ClassPrivate(GPendingIdentity.Class),
      NamePrivate(GPendingIdentity.Name), OuterPrivate(GPendingIdentity.Outer) {
    UE5PARSE_CHECK(GPendingIdentity.Object == this);
    GPendingIdentity = FPendingIdentity();
}

UObjectBase::~UObjectBase() = default;

void GetObjectsWithOuter(const UObject* Outer, TArray<UObject*>& Results, bool bIncludeNestedObjects) {
    FObjectHashTables& Tables = GetObjectHashTables();
    std::lock_guard Lock(Tables.Mutex);
    Results.Reset();
    auto Found = Tables.ByOuter.find(Outer);
    if (Found == Tables.ByOuter.end()) return;
    Results = Found->second;
    if (!bIncludeNestedObjects) return;
    for (int32 Index = 0; Index < Results.Num(); Index++) {
        auto Nested = Tables.ByOuter.find(Results[Index]);
        if (Nested == Tables.ByOuter.end()) continue;
        for (UObject* Object : Nested->second) Results.Add(Object);
    }
}

void SetPendingIdentity(UObjectBase* Object, UClass* Class, UObject* Outer, FName Name, EObjectFlags Flags) {
    UE5PARSE_CHECK(GPendingIdentity.Object == nullptr);
    GPendingIdentity.Object = Object;
    GPendingIdentity.Flags = Flags | RF_NeedInitialization;
    GPendingIdentity.InternalIndex = GUObjectArray.AllocateIndex(Object);
    GPendingIdentity.Class = Class;
    GPendingIdentity.Name = Name;
    GPendingIdentity.Outer = Outer;
    // Findable by name from now on, so that a loading object finds the subobjects its constructor makes.
    GetObjectHashTables().Add(static_cast<UObject*>(Object), Outer, Name);
}

UObject* StaticAllocateObject(const UClass* Class, UObject* Outer, FName Name, EObjectFlags SetFlags) {
    UE5PARSE_CHECK(Class != nullptr && Class->IsLinked());
    if (Name.isNone()) Name = MakeUniqueObjectName(Outer, Class);
    int32 Size = std::max(Class->GetPropertiesSize(), Class->GetNativeSize());
    int32 Alignment = std::max(Class->GetMinAlignment(), 16);
    void* Memory = RuntimeMalloc(static_cast<std::size_t>(Size), static_cast<std::size_t>(Alignment));
    std::memset(Memory, 0, static_cast<std::size_t>(Size));
    UObject* Object = static_cast<UObject*>(Memory);
    SetPendingIdentity(Object, const_cast<UClass*>(Class), Outer, Name, SetFlags);
    return Object;
}

FObjectInitializer::FObjectInitializer(UObject* InObj, UObject* InObjectArchetype, bool bInCopyTransientsFromClassDefaults,
                                       bool bInShouldInitializeProps, FObjectInstancingGraph* InInstanceGraph)
    : Obj(InObj), ObjectArchetype(InObjectArchetype), bCopyTransientsFromClassDefaults(bInCopyTransientsFromClassDefaults),
      bShouldInitializePropsFromArchetype(bInShouldInitializeProps), InstanceGraph(InInstanceGraph), PreviousInitializer(GTopInitializer) {
    GTopInitializer = this;
}

FObjectInitializer::~FObjectInitializer() {
    UClass* Class = Obj->GetClass();
    if (ObjectArchetype == nullptr && bShouldInitializePropsFromArchetype && !Obj->HasAnyFlags(RF_ClassDefaultObject))
        ObjectArchetype = Class->GetDefaultObject();
    PostConstructInit();
    GTopInitializer = PreviousInitializer;
}

FObjectInitializer& FObjectInitializer::Get() {
    UE5PARSE_CHECK(GTopInitializer != nullptr);
    return *GTopInitializer;
}

UClass* FObjectInitializer::GetClass() const { return Obj->GetClass(); }

namespace {

bool IsTransientProperty(const FProperty* Property) {
    return Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient);
}

bool ContainsInstancedObjectProperty(const FProperty* Property) {
    return Property->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference);
}

}

void FObjectInitializer::InitProperties(UObject* Obj, UClass* DefaultsClass, UObject* DefaultData, bool bCopyTransientsFromClassDefaults) {
    UClass* Class = Obj->GetClass();
    bool bNeedInitialize = !Class->HasAnyClassFlags(CLASS_Native | CLASS_Intrinsic);
    bool bCanUsePostConstructLink = !bCopyTransientsFromClassDefaults && DefaultsClass == Class;
    if (Obj->HasAnyFlags(RF_NeedLoad)) bCopyTransientsFromClassDefaults = false;
    if (DefaultData == nullptr) return;
    int32 DefaultsSize = DefaultsClass->GetPropertiesSize();

    if (!bNeedInitialize && bCanUsePostConstructLink) {
        // A native object from its class default object: C++ set the same values, only the others are copied.
        bool bFromClassDefaults = Class->GetDefaultObject(false) == DefaultData;
        for (FProperty* P = bFromClassDefaults ? Class->PostConstructLink : Class->PropertyLink; P != nullptr;
             P = bFromClassDefaults ? P->PostConstructLinkNext : P->PropertyLinkNext) {
            if (IsTransientProperty(P) && ContainsInstancedObjectProperty(P)) continue;
            if (P->IsInContainer(DefaultsSize)) P->CopyCompleteValue_InContainer(Obj, DefaultData);
        }
        return;
    }

    // Every property.
    bCanUsePostConstructLink &= DefaultData == Class->GetDefaultObject(false);
    UObject* ClassDefaults = bCopyTransientsFromClassDefaults ? DefaultsClass->GetDefaultObject() : nullptr;
    for (FProperty* P = bCanUsePostConstructLink ? Class->PostConstructLink : Class->PropertyLink; P != nullptr;
         P = bCanUsePostConstructLink ? P->PostConstructLinkNext : P->PropertyLinkNext) {
        bool bIsTransient = IsTransientProperty(P);
        if (bIsTransient && ContainsInstancedObjectProperty(P)) continue;
        if (bCopyTransientsFromClassDefaults && bIsTransient && ClassDefaults != nullptr) {
            P->CopyCompleteValue_InContainer(Obj, ClassDefaults);
        } else if (P->IsInContainer(DefaultsSize)) {
            P->CopyCompleteValue_InContainer(Obj, DefaultData);
        }
    }
    // The PostConstructLink chain has no property that C++ owns.
    if (bCanUsePostConstructLink) Class->InitPropertiesFromCustomList(reinterpret_cast<uint8*>(Obj), reinterpret_cast<const uint8*>(DefaultData));
}

UObject* FObjectInitializer::CreateDefaultSubobject(UObject* Outer, FName SubobjectFName, UClass* ReturnType,
                                                    UClass* ClassToCreateByDefault, bool bIsRequired, bool bIsTransient) const {
    // The archetype's subobject of the name is the template; its class wins over the default class (a Blueprint can
    // replace a native component's class).
    UObject* OuterArchetype = Outer == Obj ? ObjectArchetype : Outer->GetArchetype();
    UObject* Template = OuterArchetype ? StaticFindObjectFast(nullptr, OuterArchetype, SubobjectFName) : nullptr;
    // The subclass overrides of the object's own subobjects (UE's FOverrides::Get).
    if (Outer == Obj)
        for (const FSubobjectOverride& Override : SubobjectOverrides) {
            if (Override.Name != SubobjectFName) continue;
            if (Override.Class == nullptr) {
                if (!bIsRequired) return nullptr;
            } else if (Override.Class->IsChildOf(ReturnType)) {
                ClassToCreateByDefault = Override.Class;
            }
        }
    // A template of a more derived class keeps its class (the cook names the class it made); one of a super of the class
    // to create, the super class's subobject that a subclass constructor overrode, stays the template of the override.
    UClass* Class = ClassToCreateByDefault;
    if (Template != nullptr && Template->GetClass()->IsChildOf(ReturnType)) {
        if (!Class->IsChildOf(Template->GetClass())) Class = Template->GetClass();
    } else {
        Template = nullptr;
    }
    if (Class == nullptr || (!bIsRequired && Template == nullptr && OuterArchetype != nullptr && !OuterArchetype->HasAnyFlags(RF_ClassDefaultObject)))
        return nullptr;

    // A loading object finds the subobject its cooked data made (the linker created it first), or makes it now and the
    // linker reuses it.
    if (UObject* Existing = StaticFindObjectFast(nullptr, Outer, SubobjectFName)) return Existing;

    EObjectFlags SubobjectFlags = (Outer->GetFlags() & RF_PropagateToSubObjects) | RF_DefaultSubObject;
    if (bIsTransient) SubobjectFlags = SubobjectFlags | RF_Transient;
    if (Outer->HasAnyFlags(RF_NeedLoad)) SubobjectFlags = SubobjectFlags | RF_NeedLoad;
    FStaticConstructObjectParameters Params;
    Params.Class = Class;
    Params.Outer = Outer;
    Params.Name = SubobjectFName;
    Params.SetFlags = SubobjectFlags;
    // A template of a super class (an overridden class's) is no archetype: the object starts from its class default
    // object and takes the template's values afterwards, as far as the template's class has them (UE's ComponentInits).
    Params.Template = Template != nullptr && Template->GetClass() == Class ? Template : nullptr;
    UObject* Result = StaticConstructObject_Internal(Params);
    if (Template != nullptr) SubobjectInits.Add(FSubobjectInit{Result, Template});
    return Result;
}

UObject* FObjectInitializer::CreateDefaultSubobject(FName SubobjectName, const char* ClassName, bool bIsRequired, bool bIsTransient) const {
    UClass* Class = FindNativeClass(FName(ClassName));
    if (Class == nullptr) {
        ReportLoadProblem("load.subobject_class", FString(ClassName));
        return nullptr;
    }
    return CreateDefaultSubobject(Obj, SubobjectName, Class, Class, bIsRequired, bIsTransient);
}

void FObjectInitializer::PostConstructInit() {
    bool bIsCDO = Obj->HasAnyFlags(RF_ClassDefaultObject);
    UClass* Class = Obj->GetClass();
    UClass* SuperClass = Class->GetSuperClass();
    // No C++ constructor makes the properties of the PostConstructLink chain: those of a Blueprint class, and the mapped
    // properties that a native class's C++ type does not have. Their memory is only zeroed, and a zeroed FName is not
    // None.
    for (FProperty* P = Class->PostConstructLink; P != nullptr; P = P->PostConstructLinkNext)
        if (!P->HasAnyPropertyFlags(CPF_ZeroConstructor)) P->InitializeValue_InContainer(Obj);
    if (bShouldInitializePropsFromArchetype) {
        UClass* BaseClass = bIsCDO ? SuperClass : Class;
        if (BaseClass == nullptr) BaseClass = Class;
        UObject* Defaults = ObjectArchetype ? ObjectArchetype : BaseClass->GetDefaultObject(false);
        InitProperties(Obj, BaseClass, Defaults, bCopyTransientsFromClassDefaults);
    }
    for (const FSubobjectInit& Init : SubobjectInits) InitProperties(Init.Subobject, Init.Template->GetClass(), Init.Template, false);

    // Instancing: references to subobjects of the archetype become references to this object's own subobjects. Not
    // for loading objects: their cooked data names their subobjects.
    if (!Obj->HasAnyFlags(RF_NeedLoad) && ObjectArchetype != nullptr && !bIsCDO) {
        FObjectInstancingGraph LocalGraph(ObjectArchetype, Obj);
        FObjectInstancingGraph* Graph = InstanceGraph ? InstanceGraph : &LocalGraph;
        Graph->AddNewObject(Obj, ObjectArchetype);
        for (const FSubobjectInit& Init : SubobjectInits) Graph->AddNewObject(Init.Subobject, Init.Template);
        // The archetype's default subobjects that no C++ constructor here made (native classes the runtime has no C++
        // for): made from their templates, as those constructors would.
        TArray<UObject*> ArchetypeSubobjects;
        GetObjectsWithOuter(ObjectArchetype, ArchetypeSubobjects, false);
        for (UObject* Template : ArchetypeSubobjects) {
            if (!Template->HasAnyFlags(RF_DefaultSubObject) || Graph->GetDestinationObject(Template) != nullptr) continue;
            if (UObject* Existing = StaticFindObjectFast(nullptr, Obj, Template->GetFName())) {
                Graph->AddNewObject(Existing, Template);
                continue;
            }
            Graph->InstancePropertyValue(Template, Obj, false, true);
        }
        InstanceSubobjectTemplates(Class, Obj, ObjectArchetype, ObjectArchetype->GetClass(), Obj, Graph);
        for (const FSubobjectInit& Init : SubobjectInits)
            InstanceSubobjectTemplates(Init.Subobject->GetClass(), Init.Subobject, Init.Template, Init.Template->GetClass(),
                                       Init.Subobject, Graph);
    }
    Obj->PostInitProperties();
    Class->PostInitInstance(Obj);
    Obj->ClearFlags(RF_NeedInitialization);
}

void FObjectInstancingGraph::AddNewObject(UObject* ObjectInstance, UObject* InArchetype) {
    if (InArchetype == nullptr) return;
    for (auto& [Source, Destination] : SourceToDestinationMap)
        if (Source == InArchetype) {
            Destination = ObjectInstance;
            return;
        }
    SourceToDestinationMap.Add({InArchetype, ObjectInstance});
}

UObject* FObjectInstancingGraph::GetDestinationObject(UObject* SourceObject) const {
    if (SourceObject == SourceRoot) return DestinationRoot;
    for (const auto& [Source, Destination] : SourceToDestinationMap)
        if (Source == SourceObject) return Destination;
    return nullptr;
}

UObject* FObjectInstancingGraph::InstancePropertyValue(UObject* ComponentTemplate, UObject* Owner, bool bIsTransient,
                                                       bool bCausesInstancing) {
    if (ComponentTemplate == nullptr || SourceRoot == nullptr || !ComponentTemplate->IsInOuter(SourceRoot)) return ComponentTemplate;
    if (UObject* Existing = GetDestinationObject(ComponentTemplate)) return Existing;
    if (!bCausesInstancing || !bEnableSubobjectInstancing) return nullptr;
    // The copy goes into the instance of the template's outer.
    UObject* DestinationOuter = GetDestinationObject(ComponentTemplate->GetOuter());
    if (DestinationOuter == nullptr) DestinationOuter = InstancePropertyValue(ComponentTemplate->GetOuter(), Owner, bIsTransient, true);
    if (DestinationOuter == nullptr) return nullptr;
    if (UObject* Existing = StaticFindObjectFast(nullptr, DestinationOuter, ComponentTemplate->GetFName())) {
        AddNewObject(Existing, ComponentTemplate);
        return Existing;
    }
    FStaticConstructObjectParameters Params;
    Params.Class = ComponentTemplate->GetClass();
    Params.Outer = DestinationOuter;
    Params.Name = ComponentTemplate->GetFName();
    Params.SetFlags = (DestinationOuter->GetFlags() & RF_PropagateToSubObjects) | (ComponentTemplate->GetFlags() & RF_DefaultSubObject);
    if (bIsTransient) Params.SetFlags = Params.SetFlags | RF_Transient;
    Params.Template = ComponentTemplate;
    Params.InstanceGraph = this;
    // Registered before construction, so that references to it inside its own construction resolve.
    UObject* Instance = StaticConstructObject_Internal(Params);
    AddNewObject(Instance, ComponentTemplate);
    return Instance;
}

namespace {

void InstanceValue(const FProperty* Property, void* Data, const void* DefaultData, UObject* Owner, FObjectInstancingGraph* Graph);

void InstanceStruct(const UStruct* Struct, void* Data, const void* DefaultData, UObject* Owner, FObjectInstancingGraph* Graph) {
    for (FProperty* Property = Struct->RefLink; Property != nullptr; Property = Property->NextRef) {
        for (int32 i = 0; i < Property->ArrayDim; i++) {
            void* Value = Property->ContainerPtrToValuePtr<void>(Data, i);
            const void* Default = DefaultData ? Property->ContainerPtrToValuePtr<void>(DefaultData, i) : nullptr;
            InstanceValue(Property, Value, Default, Owner, Graph);
        }
    }
}

void InstanceValue(const FProperty* Property, void* Data, const void* DefaultData, UObject* Owner, FObjectInstancingGraph* Graph) {
    if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property)) {
        if (Property->IsA<FSoftObjectProperty>() || Property->IsA<FWeakObjectProperty>()) return;
        UObject* Current = Object->GetObjectPropertyValue(Data);
        if (Current == nullptr) return;
        UObject* NewValue = Graph->InstancePropertyValue(Current, Owner, Property->HasAnyPropertyFlags(CPF_Transient), true);
        if (NewValue != Current) Object->SetObjectPropertyValue(Data, NewValue);
    } else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property)) {
        InstanceStruct(StructProperty->Struct, Data, DefaultData, Owner, Graph);
    } else if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) {
        if (!Array->Inner->IsA<FObjectPropertyBase>() && !Array->Inner->IsA<FStructProperty>()) return;
        FScriptArrayHelper Helper(Array, Data);
        for (int32 i = 0; i < Helper.Num(); i++) InstanceValue(Array->Inner, Helper.GetRawPtr(i), nullptr, Owner, Graph);
    } else if (const FMapProperty* Map = CastField<FMapProperty>(Property)) {
        if (!Map->ValueProp->IsA<FObjectPropertyBase>() && !Map->ValueProp->IsA<FStructProperty>()) return;
        FScriptMapHelper Helper(Map, Data);
        for (int32 i = 0; i < Helper.GetMaxIndex(); i++)
            if (Helper.IsValidIndex(i)) InstanceValue(Map->ValueProp, Helper.GetValuePtr(i), nullptr, Owner, Graph);
    }
}

}

void InstanceSubobjectTemplates(const UStruct* Struct, void* Data, const void* DefaultData, const UStruct* DefaultStruct,
                                UObject* Owner, FObjectInstancingGraph* InstanceGraph) {
    (void)DefaultStruct;
    InstanceStruct(Struct, Data, DefaultData, Owner, InstanceGraph);
}

UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params) {
    std::lock_guard Lock(GetLoaderMutex());
    UObject* Template = Params.Template;
    UObject* Result = StaticAllocateObject(Params.Class, Params.Outer, Params.Name, Params.SetFlags);
    (*Params.Class->ClassConstructor)(FObjectInitializer(Result, Template, Params.bCopyTransientsFromClassDefaults, true, Params.InstanceGraph));
    return Result;
}

FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName) {
    // "BaseName_N" with N from the class's counter, as UE: the first one is "_0".
    UClass* CounterClass = const_cast<UClass*>(Class);
    FName Base = BaseName.isNone() ? (Class ? Class->GetFName() : MakeName("Object")) : BaseName;
    Base = MakeName(Base.getText());
    int32 LocalCounter = 0;
    for (;;) {
        int32 Number = CounterClass ? ++CounterClass->ClassUnique : ++LocalCounter;
        FName Candidate(Base.getText(), Base.getHash(), static_cast<uint32>(Number));
        if (StaticFindObjectFast(nullptr, Outer, Candidate) == nullptr) return Candidate;
    }
}

UObject* StaticFindObjectFast(const UClass* Class, UObject* Outer, FName Name, bool bExactClass) {
    FObjectHashTables& Tables = GetObjectHashTables();
    std::lock_guard Lock(Tables.Mutex);
    auto Found = Tables.ByName.find(FObjectKey{Outer, Name});
    if (Found == Tables.ByName.end()) return nullptr;
    UObject* Object = Found->second;
    if (Object->IsGarbage()) return nullptr;
    if (Class != nullptr && (bExactClass ? Object->GetClass() != Class : !Object->IsA(Class))) return nullptr;
    return Object;
}

namespace {

// Splits "/Game/A/B.B:Sub.Inner" into the package name and the object names.
bool SplitObjectPath(const FString& Path, FString& PackageName, TArray<FString>& ObjectNames) {
    ObjectNames.Reset();
    int32 Dot = Path.Find(FString("."), ESearchCase::CaseSensitive);
    PackageName = Dot == INDEX_NONE ? Path : Path.Left(Dot);
    if (Dot == INDEX_NONE) return !PackageName.IsEmpty();
    FString Rest = Path.Mid(Dot + 1);
    int32 Start = 0;
    for (int32 i = 0; i <= Rest.Len(); i++) {
        if (i == Rest.Len() || Rest[i] == u'.' || Rest[i] == u':') {
            if (i > Start) ObjectNames.Add(Rest.Mid(Start, i - Start));
            Start = i + 1;
        }
    }
    return true;
}

UObject* FindNativeScriptObject(const FString& PackageName, const TArray<FString>& ObjectNames) {
    if (ObjectNames.Num() == 0) return FindPackage(PackageName);
    FName First = MakeName(ObjectNames[0]);
    UObject* Object = FindNativeClass(First);
    if (Object == nullptr) Object = FindNativeStruct(First);
    if (Object == nullptr) Object = FindNativeEnum(First);
    // "Default__Actor"
    if (Object == nullptr && ObjectNames[0].StartsWith(FString("Default__"), ESearchCase::CaseSensitive)) {
        if (UClass* Class = FindNativeClass(MakeName(ObjectNames[0].Mid(9)))) Object = Class->GetDefaultObject();
    }
    for (int32 i = 1; Object != nullptr && i < ObjectNames.Num(); i++) {
        FName Name = MakeName(ObjectNames[i]);
        UObject* Child = StaticFindObjectFast(nullptr, Object, Name);
        if (Child == nullptr)
            if (UClass* Class = Cast<UClass>(Object)) Child = Class->FindFunctionByName(Name, false);
        Object = Child;
    }
    return Object;
}

}

UObject* StaticFindObject(const UClass* Class, UObject* Outer, const FString& Path, bool bExactClass) {
    if (Outer != nullptr && Path.Find(FString("."), ESearchCase::CaseSensitive) == INDEX_NONE &&
        Path.Find(FString(":"), ESearchCase::CaseSensitive) == INDEX_NONE)
        return StaticFindObjectFast(Class, Outer, MakeName(Path), bExactClass);
    FString PackageName;
    TArray<FString> ObjectNames;
    if (!SplitObjectPath(Path, PackageName, ObjectNames)) return nullptr;
    UObject* Object;
    if (PackageName.StartsWith(FString("/Script/"), ESearchCase::IgnoreCase)) {
        Object = FindNativeScriptObject(PackageName, ObjectNames);
    } else {
        Object = FindPackage(PackageName);
        for (int32 i = 0; Object != nullptr && i < ObjectNames.Num(); i++) Object = StaticFindObjectFast(nullptr, Object, MakeName(ObjectNames[i]));
    }
    if (Object == nullptr || Class == nullptr) return Object;
    return (bExactClass ? Object->GetClass() == Class : Object->IsA(Class)) ? Object : nullptr;
}

UObject* StaticLoadObject(const UClass* Class, UObject* Outer, const FString& Path) {
    if (UObject* Found = StaticFindObject(Class, Outer, Path)) return Found;
    FString PackageName;
    TArray<FString> ObjectNames;
    if (!SplitObjectPath(Path, PackageName, ObjectNames)) return nullptr;
    if (PackageName.StartsWith(FString("/Script/"), ESearchCase::IgnoreCase)) return nullptr;
    std::lock_guard Lock(GetLoaderMutex());
    FLoadScope Scope;
    FLinkerLoad* Linker = FLinkerLoad::Load(PackageName);
    if (Linker == nullptr) return nullptr;
    FString ObjectPath;
    for (int32 i = 0; i < ObjectNames.Num(); i++) {
        if (i > 0) ObjectPath += i == 1 ? u":" : u".";
        ObjectPath += ObjectNames[i];
    }
    if (ObjectNames.Num() == 0) return Linker->GetLinkerRoot();
    int32 Index = Linker->FindExportIndex(ObjectPath);
    if (Index == INDEX_NONE) return nullptr;
    UObject* Object = Linker->CreateExport(Index);
    if (Object == nullptr || Class == nullptr) return Object;
    return Object->IsA(Class) ? Object : nullptr;
}

UClass* StaticLoadClass(const UClass* BaseClass, UObject* Outer, const FString& Path) {
    UClass* Class = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), Outer, Path));
    if (Class != nullptr && BaseClass != nullptr && !Class->IsChildOf(BaseClass)) return nullptr;
    return Class;
}

UPackage* FindPackage(const FString& PackageName) {
    return Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), nullptr, MakeName(PackageName)));
}

UPackage* CreatePackage(const FString& PackageName) {
    std::lock_guard Lock(GetLoaderMutex());
    if (UPackage* Existing = FindPackage(PackageName)) return Existing;
    return NewObject<UPackage>(nullptr, UPackage::StaticClass(), MakeName(PackageName), RF_Public);
}

UPackage* LoadPackage(const FString& PackageName) {
    std::lock_guard Lock(GetLoaderMutex());
    FLoadScope Scope;
    FLinkerLoad* Linker = FLinkerLoad::Load(PackageName);
    if (Linker == nullptr) return nullptr;
    Linker->LoadAllObjects();
    return Linker->GetLinkerRoot();
}

UPackage* GetTransientPackage() {
    if (GTransientPackage == nullptr) GTransientPackage = CreatePackage(FString("/Engine/Transient"));
    return GTransientPackage;
}

void DestroyObject(UObjectBase* Base) {
    UObject* Object = static_cast<UObject*>(Base);
    if (!Object->HasAnyFlags(RF_BeginDestroyed)) Object->BeginDestroy();
    if (!Object->HasAnyFlags(RF_FinishDestroyed)) Object->FinishDestroy();
    GetObjectHashTables().Remove(Object);
    UClass* Class = Object->GetClass();
    // UE's UObject::DestroyNonNativeProperties.
    Class->DestroyPersistentUberGraphFrame(Object);
    // The properties that no C++ destructor owns.
    for (FProperty* P = Class->DestructorLink; P != nullptr; P = P->DestructorLinkNext) P->DestroyValue_InContainer(Object);
    int32 Index = Object->GetInternalIndex();
    Object->~UObject();
    GUObjectArray.FreeIndex(Index);
    RuntimeFree(Object);
}

void RenameObject(UObjectBase* Base, FName NewName, UObject* NewOuter) {
    UObject* Object = static_cast<UObject*>(Base);
    GetObjectHashTables().Remove(Object);
    Object->NamePrivate = NewName;
    Object->OuterPrivate = NewOuter;
    GetObjectHashTables().Add(Object, NewOuter, NewName);
}

namespace {

void ClearGarbageReferences(const FProperty* Property, void* Data);

void ClearGarbageReferencesInStruct(const UStruct* Struct, void* Data) {
    for (FProperty* Property = Struct->RefLink; Property != nullptr; Property = Property->NextRef)
        for (int32 i = 0; i < Property->ArrayDim; i++) ClearGarbageReferences(Property, Property->ContainerPtrToValuePtr<void>(Data, i));
}

bool IsGarbageOrInGarbage(const UObject* Object) {
    for (const UObject* Current = Object; Current != nullptr; Current = Current->GetOuter())
        if (Current->IsGarbage()) return true;
    return false;
}

void ClearGarbageReferences(const FProperty* Property, void* Data) {
    if (const FObjectProperty* Object = CastField<FObjectProperty>(Property)) {
        UObject* Value = Object->GetObjectPropertyValue(Data);
        if (Value != nullptr && IsGarbageOrInGarbage(Value)) Object->SetObjectPropertyValue(Data, nullptr);
    } else if (CastField<FInterfaceProperty>(Property) != nullptr) {
        FScriptInterface& Value = *static_cast<FScriptInterface*>(Data);
        if (Value.GetObject() != nullptr && IsGarbageOrInGarbage(Value.GetObject())) Value.SetObject(nullptr);
    } else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property)) {
        ClearGarbageReferencesInStruct(StructProperty->Struct, Data);
    } else if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) {
        FScriptArrayHelper Helper(Array, Data);
        for (int32 i = 0; i < Helper.Num(); i++) ClearGarbageReferences(Array->Inner, Helper.GetRawPtr(i));
    } else if (const FMapProperty* Map = CastField<FMapProperty>(Property)) {
        FScriptMapHelper Helper(Map, Data);
        for (int32 i = 0; i < Helper.GetMaxIndex(); i++)
            if (Helper.IsValidIndex(i)) ClearGarbageReferences(Map->ValueProp, Helper.GetValuePtr(i));
    }
}

}

void CollectGarbage() {
    std::lock_guard Lock(GetLoaderMutex());
    TArray<UObject*> Garbage;
    TArray<UObject*> Live;
    for (int32 Index = 0; Index < GUObjectArray.GetObjectArrayNum(); Index++) {
        FUObjectItem* Item = GUObjectArray.IndexToObject(Index);
        if (Item == nullptr || Item->Object == nullptr) continue;
        UObject* Object = static_cast<UObject*>(Item->Object);
        if (IsGarbageOrInGarbage(Object) && !Object->IsA<UField>()) {
            Garbage.Add(Object);
        } else {
            Live.Add(Object);
        }
    }
    if (Garbage.Num() == 0) return;
    for (UObject* Object : Live) ClearGarbageReferencesInStruct(Object->GetClass(), Object);
    for (UObject* Object : Garbage)
        if (!Object->HasAnyFlags(RF_BeginDestroyed)) Object->BeginDestroy();
    // Inner objects first.
    for (int32 i = Garbage.Num() - 1; i >= 0; i--) DestroyObject(Garbage[i]);
}

}
