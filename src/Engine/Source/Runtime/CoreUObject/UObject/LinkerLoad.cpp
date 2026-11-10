#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Stack.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

ankerl::unordered_dense::map<const ue5parse::Package*, std::unique_ptr<FLinkerLoad>>& GetLinkers() {
    static ankerl::unordered_dense::map<const ue5parse::Package*, std::unique_ptr<FLinkerLoad>> Linkers;
    return Linkers;
}

FLoadProblemHandler GLoadProblemHandler = nullptr;

struct FPackedRefRanges {
    std::vector<std::uintptr_t> Bases;
    std::vector<FLinkerLoad*> Linkers;
    std::uintptr_t Next = 0;
};

FPackedRefRanges& GetPackedRefRanges() {
    static FPackedRefRanges Ranges;
    return Ranges;
}

template <typename FIndex>
ue5parse::ResolvedObject ResolveIndex(const ue5parse::Package& Package, const FIndex Index) {
    if (!Package.resolveLoads(Index))
        return Package.resolve(Index);
    FLoaderUnlock Unlock;
    return Package.resolve(Index);
}

constexpr uint32 LoadFlagsMask = RF_Public | RF_Standalone | RF_Transactional | RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient | RF_DefaultSubObject
                                 | RF_InheritableComponentTemplate;

thread_local int32 GLoadScopeDepth = 0;
thread_local TArray<UObject*>* GLoadedObjects = nullptr;

struct FLoadingThread {
    const FLoadingThread* WaitingFor = nullptr;
};
thread_local FLoadingThread GLoadingThread;

ankerl::unordered_dense::map<const UObject*, const FLoadingThread*> GLoadingObjects;

void ExecMissingFunction(UObject* Context, FFrame& Stack, RESULT_DECL) {
    Stack.SkipParms();
    UFunction* Function = Stack.CurrentNativeFunction;
    FString Message = FString("missing native function ") + (Function ? Function->GetPathName() : FString("?"));
    Stack.KismetExecutionMessage("MissingNative", Message.ToUtf8(), EBlueprintExceptionType::NonFatalError);
    (void)Context;
    (void)RESULT_PARAM;
}

}

void ReportLoadProblem(const char* Code, const FString& Message) {
    static std::mutex Mutex;
    static std::set<std::string> Reported;
    std::string Key = std::string(Code) + ' ' + Message.ToUtf8();
    {
        std::lock_guard Lock(Mutex);
        if (!Reported.insert(Key).second)
            return;
    }
    if (GLoadProblemHandler != nullptr)
        GLoadProblemHandler(Code, Message);
}

void SetLoadProblemHandler(FLoadProblemHandler Handler) {
    GLoadProblemHandler = Handler;
}

namespace {

struct FDeferredExport {
    FLinkerLoad* Linker;
    int32 Index;
    const UClass* Class;
    const FLoadingThread* Thread;
    std::vector<std::pair<const FObjectPropertyBase*, void*>> References;
};

std::vector<FDeferredExport> GDeferredExports;
thread_local int32 GLastDeferredExport = INDEX_NONE;

int32 FindDeferredExport(const FLinkerLoad* Linker, int32 Index) {
    for (int32 i = 0; i < static_cast<int32>(GDeferredExports.size()); ++i)
        if (GDeferredExports[static_cast<std::size_t>(i)].Linker == Linker && GDeferredExports[static_cast<std::size_t>(i)].Index == Index)
            return i;
    return INDEX_NONE;
}

}

FLoadScope::FLoadScope() {
    if (GLoadScopeDepth++ == 0)
        GLoadedObjects = new TArray<UObject*>();
}

FLoadScope::~FLoadScope() {
    if (--GLoadScopeDepth != 0)
        return;
    for (const FDeferredExport& Deferred : GDeferredExports) {
        if (Deferred.Thread != &GLoadingThread)
            continue;
        const UClass* Unlinked = Deferred.Class;
        while (Unlinked->GetSuperClass() != nullptr && !Unlinked->GetSuperClass()->IsLinked())
            Unlinked = Unlinked->GetSuperClass();
        ReportLoadProblem("load.class_unlinked", FString(Deferred.Linker->GetPackage().getExportPathName(static_cast<std::size_t>(Deferred.Index)))
                                                     + FString(" (") + Unlinked->GetPathName() + FString(")"));
    }
    TArray<UObject*>* Loaded = GLoadedObjects;
    GLoadScopeDepth++;
    for (int32 i = 0; i < Loaded->Num(); i++)
        (*Loaded)[i]->ConditionalPostLoad();
    GLoadScopeDepth--;
    GLoadedObjects = nullptr;
    for (const UObject* Object : *Loaded)
        GLoadingObjects.erase(Object);
    delete Loaded;
    GetLoaderMutex().NotifyAll();
}

void WaitForLoad(const UObject* Object) {
    const auto Found = GLoadingObjects.find(Object);
    if (Found == GLoadingObjects.end())
        return;
    //! A thread that would close a cycle sees it here and does not wait, so the waiting threads never form one.
    for (const FLoadingThread* Thread = Found->second; Thread != nullptr; Thread = Thread->WaitingFor)
        if (Thread == &GLoadingThread)
            return;
    GLoadingThread.WaitingFor = Found->second;
    GetLoaderMutex().Wait([Object] { return !GLoadingObjects.contains(Object); });
    GLoadingThread.WaitingFor = nullptr;
}

void FLoadScope::AddLoadedObject(UObject* Object) {
    Object->SetFlags(RF_NeedPostLoad);
    if (GLoadedObjects != nullptr)
        GLoadedObjects->Add(Object);
    else
        Object->ConditionalPostLoad();
}

FLinkerLoad* FLinkerLoad::FindOrCreate(const ue5parse::Package& Package) {
    std::lock_guard Lock(GetLoaderMutex());
    auto& Linkers = GetLinkers();
    auto Found = Linkers.find(&Package);
    if (Found != Linkers.end())
        return Found->second.get();
    auto Linker = std::make_unique<FLinkerLoad>(Package);
    FLinkerLoad* Result = Linker.get();
    Linkers.emplace(&Package, std::move(Linker));
    return Result;
}

FLinkerLoad* FLinkerLoad::Load(const FString& PackageName) {
    Result<const ue5parse::Package*> Package = [&] {
        FLoaderUnlock Unlock;
        return GetRuntimeProvider().loadPackage(PackageName.ToUtf8());
    }();
    if (!Package)
        return nullptr;
    return FindOrCreate(**Package);
}

void FLinkerLoad::ResetAll() {
    std::lock_guard Lock(GetLoaderMutex());
    GetLoaderMutex().WaitForPausedLoads();
    FPackedRefRanges& Ranges = GetPackedRefRanges();
    Ranges.Bases.clear();
    Ranges.Linkers.clear();
    GetLinkers().clear();
}

FLinkerLoad::FLinkerLoad(const ue5parse::Package& InPackage) : Package(&InPackage) {
    LinkerRoot = CreatePackage(FString(Package->getName().toString()));
    int32 Count = static_cast<int32>(Package->getExportCount());
    {
        std::lock_guard Lock(GetLoaderMutex());
        FPackedRefRanges& Ranges = GetPackedRefRanges();
        const std::uintptr_t Slots = Package->getHeader().getImportMap().size() + 1 + static_cast<std::uintptr_t>(Count);
        UE5PARSE_CHECK(Slots <= (UINTPTR_MAX >> 1) - Ranges.Next);
        PackedRefBase = Ranges.Next;
        Ranges.Bases.push_back(PackedRefBase);
        Ranges.Linkers.push_back(this);
        Ranges.Next += Slots;
    }
    ExportObjects.SetNum(Count);
    ExportFailed.SetNum(Count);
    DefaultSubobjectExports.SetNum(Count);
    for (int32 i = 0; i < Count; i++) {
        const ue5parse::FExportMapEntry& Entry = Package->getExportMapEntry(static_cast<std::size_t>(i));
        if (!(Entry.objectFlags & RF_DefaultSubObject) || Entry.outerIndex.isNull())
            continue;
        const ue5parse::ResolvedObject Outer = Package->resolve(Entry.outerIndex);
        if (!Outer.isExport() || Outer.getPackage() != Package)
            continue;
        const int32 OuterIndex = static_cast<int32>(Outer.getExportIndex());
        if (OuterIndex >= 0 && OuterIndex < Count)
            DefaultSubobjectExports[OuterIndex].Add(i);
    }
}

FName FLinkerLoad::GetName(uint32 NameIndex, uint32 Number) const {
    std::span<const FName> Names = Package->getHeader().getNameMap();
    if (NameIndex >= Names.size())
        return FName();
    const FName& Base = Names[NameIndex];
    return FName(Base.getText(), Base.getHash(), Number);
}

uint32 FLinkerLoad::GetNameCount() const {
    return static_cast<uint32>(Package->getHeader().getNameMap().size());
}

UObject* FLinkerLoad::GetExportTemplate(int32 Index) {
    if (Index < 0 || Index >= ExportObjects.Num())
        return nullptr;
    const ue5parse::FExportMapEntry& Entry = Package->getExportMapEntry(static_cast<std::size_t>(Index));
    if (Entry.templateIndex.isNull())
        return nullptr;
    return ResolveObject(ResolveIndex(*Package, Entry.templateIndex));
}

void BeginObjectReference() {
    GLastDeferredExport = INDEX_NONE;
}

void TrackDeferredExportReference(const FObjectPropertyBase* Property, void* Value) {
    if (GLastDeferredExport == INDEX_NONE)
        return;
    GDeferredExports[static_cast<std::size_t>(GLastDeferredExport)].References.emplace_back(Property, Value);
    GLastDeferredExport = INDEX_NONE;
}

void ResolveExportsWaitingForClass(const UClass* Class) {
    std::lock_guard Lock(GetLoaderMutex());
    //! A search from the start each time: CreateExport lets the lock go, and other threads change the list meanwhile.
    for (;;) {
        const auto Found = std::ranges::find(GDeferredExports, Class, &FDeferredExport::Class);
        if (Found == GDeferredExports.end())
            return;
        FDeferredExport Deferred = std::move(*Found);
        GDeferredExports.erase(Found);
        UObject* Object = Deferred.Linker->CreateExport(Deferred.Index);
        if (Object == nullptr)
            continue;
        for (const auto& [Property, Value] : Deferred.References)
            if (Property->GetObjectPropertyValue(Value) == nullptr)
                Property->SetObjectPropertyValue(Value, Object);
    }
}

UObject* FLinkerLoad::CreateExport(int32 Index) {
    GLastDeferredExport = INDEX_NONE;
    if (Index < 0 || Index >= ExportObjects.Num())
        return nullptr;
    std::lock_guard Lock(GetLoaderMutex());
    if (UObject* Existing = FindLoadedExport(Index))
        return Existing;
    if (ExportFailed[Index])
        return nullptr;
    if (const int32 Deferred = FindDeferredExport(this, Index); Deferred != INDEX_NONE) {
        GLastDeferredExport = Deferred;
        return nullptr;
    }
    UObject* Object = nullptr;
    {
        FLoadScope Scope;
        Object = CreateExportInternal(Index);
    }
    GLastDeferredExport = INDEX_NONE;
    if (Object != nullptr)
        return Object;
    if (const int32 Deferred = FindDeferredExport(this, Index); Deferred != INDEX_NONE) {
        GLastDeferredExport = Deferred;
        return nullptr;
    }
    ExportFailed[Index] = 1;
    return nullptr;
}

UObject* FLinkerLoad::CreateExportInternal(int32 Index) {
    const ue5parse::FExportMapEntry& Entry = Package->getExportMapEntry(static_cast<std::size_t>(Index));
    FString ExportPath = FString(Package->getExportPathName(static_cast<std::size_t>(Index)));
    UClass* Class = Cast<UClass>(ResolveObject(ResolveIndex(*Package, Entry.classIndex)));
    if (Class == nullptr) {
        ReportLoadProblem("load.class_missing", ExportPath + FString(" (") + FString(Package->resolve(Entry.classIndex).getPathName()) + FString(")"));
        return nullptr;
    }
    if (!Class->IsLinked()) {
        GDeferredExports.push_back(FDeferredExport{this, Index, Class, &GLoadingThread, {}});
        return nullptr;
    }
    const FClassConstructorType Constructor = Class->ClassConstructor;
    if (Constructor == nullptr) {
        ReportLoadProblem("load.class_constructor_missing", ExportPath + FString(" (") + Class->GetPathName() + FString(")"));
        return nullptr;
    }
    UObject* Outer = Entry.outerIndex.isNull() ? LinkerRoot : ResolveObject(ResolveIndex(*Package, Entry.outerIndex));
    if (Outer == nullptr) {
        ReportLoadProblem("load.outer_missing", ExportPath);
        return nullptr;
    }
    if (UObject* Existing = FindLoadedExport(Index))
        return Existing;
    UObject* Template = Entry.templateIndex.isNull() ? nullptr : ResolveObject(ResolveIndex(*Package, Entry.templateIndex));
    if (UObject* Existing = FindLoadedExport(Index))
        return Existing;

    //! Resolve defaults before allocation: a nested load must not replace this object's pending identity.
    UObject* Archetype = Template;
    if (Archetype == nullptr)
        Archetype = (Entry.objectFlags & RF_ClassDefaultObject) ? (Class->GetSuperClass() ? Class->GetSuperClass()->GetDefaultObject() : nullptr)
                                                                : Class->GetDefaultObject();
    Result<std::vector<std::byte>> Bytes = [&] {
        FLoaderUnlock Unlock;
        return Package->readExportBytes(static_cast<std::size_t>(Index));
    }();
    if (UObject* Existing = FindLoadedExport(Index))
        return Existing;

    FName Name = Package->getExportName(static_cast<std::size_t>(Index));
    EObjectFlags Flags = EObjectFlags(Entry.objectFlags & LoadFlagsMask);
    UObject* Object = StaticFindObjectFast(nullptr, Outer, Name);
    if (Object != nullptr && !Object->IsA(Class)) {
        ReportLoadProblem("load.subobject_class", ExportPath);
        return nullptr;
    }
    if (Object != nullptr) {
        Object->SetFlags(Flags | RF_NeedLoad | RF_WasLoaded);
        ExportObjects[Index] = Object;
        GLoadingObjects[Object] = &GLoadingThread;
    } else {
        Object = StaticAllocateObject(Class, Outer, Name, Flags | RF_NeedLoad | RF_WasLoaded);
        ExportObjects[Index] = Object;
        GLoadingObjects[Object] = &GLoadingThread;
        if (Flags & RF_ClassDefaultObject)
            Class->ClassDefaultObject = Object;
        Constructor(FObjectInitializer(Object, Archetype, false, true));
    }
    Object->Linker = this;
    Object->LinkerIndex = Index;
    SerializeExport(Object, Index, Bytes);
    Object->ClearFlags(RF_NeedLoad);
    FLoadScope::AddLoadedObject(Object);
    CreateDefaultSubobjectExports(Index);
    return Object;
}

void FLinkerLoad::CreateDefaultSubobjectExports(const int32 OuterIndex) {
    for (const int32 Subobject : DefaultSubobjectExports[OuterIndex])
        CreateExport(Subobject);
}

void FLinkerLoad::SerializeExport(UObject* Object, int32 Index, const Result<std::vector<std::byte>>& Bytes) {
    if (!Bytes) {
        ReportLoadProblem("load.export_unreadable", Object->GetPathName());
        return;
    }
    const ue5parse::ZenPackageHeader& Header = Package->getHeader();
    const std::size_t ExportIndex = static_cast<std::size_t>(Index);
    FExportArchive Ar(*this, *Bytes, static_cast<int64>(Header.getExportCookedOffset(ExportIndex)),
                      static_cast<int64>(Header.getExportChunkOffset(ExportIndex)));
    if (Object->HasAnyFlags(RF_ClassDefaultObject))
        Object->GetClass()->SerializeDefaultObject(Object, Ar);
    else
        Object->Serialize(Ar);
    if (Ar.IsError()) {
        const ue5parse::Error& Error = Ar.GetError();
        ReportLoadProblem("load.serialize", Object->GetFullName() + FString(" (") + FString(ue5parse::getErrorName(Error.code)) + FString(" at ")
                                                + FString(Error.where) + FString(")"));
    } else if (!Ar.AtEnd()) {
        ReportLoadProblem("load.unread_bytes", Object->GetClass()->GetName());
    }
}

UObject* FLinkerLoad::ResolveObject(const ue5parse::ResolvedObject& Resolved) {
    if (Resolved.isNull())
        return nullptr;
    if (Resolved.isExport()) {
        FLinkerLoad* Linker = FindOrCreate(*Resolved.getPackage());
        return Linker->CreateExport(static_cast<int32>(Resolved.getExportIndex()));
    }
    ue5parse::ResolvedObject Outer = Resolved.getOuter();
    FName Name = Resolved.getName();
    if (Outer.isNull())
        return CreatePackage(FString(Name.toString()));
    ue5parse::ResolvedObject OuterOuter = Outer.getOuter();
    if (OuterOuter.isNull()) {
        if (UClass* Class = FindNativeClass(Name))
            return Class;
        if (UScriptStruct* Struct = FindNativeStruct(Name))
            return Struct;
        if (UEnum* Enum = FindNativeEnum(Name))
            return Enum;
        if (UFunction* Delegate = FindNativeDelegateFunction(Name))
            return Delegate;
        std::string_view Text = Name.getText();
        if (Text.starts_with("Default__"))
            if (UClass* Class = FindNativeClass(FName(Text.substr(9))))
                return Class->GetDefaultObject();
        ReportLoadProblem("load.native_missing", FString(Resolved.getPathName()));
        return nullptr;
    }
    UObject* OuterObject = ResolveObject(Outer);
    UClass* OuterClass = Cast<UClass>(OuterObject);
    if (OuterClass == nullptr) {
        if (OuterObject != nullptr)
            if (UObject* Child = StaticFindObjectFast(nullptr, OuterObject, Name))
                return Child;
        ReportLoadProblem("load.native_missing", FString(Resolved.getPathName()));
        return nullptr;
    }
    if (UFunction* Function = OuterClass->FindFunctionByName(Name, false))
        return Function;
    if (UObject* Child = StaticFindObjectFast(nullptr, OuterClass, Name))
        return Child;
    ReportLoadProblem("load.native_function_missing", FString(Resolved.getPathName()));
    std::string KeptName(Name.toString());
    UFunction* Stub = NewObject<UFunction>(OuterClass, UFunction::StaticClass(), MakeName(KeptName), RF_Public);
    Stub->FunctionFlags = FUNC_Native;
    Stub->Func = &ExecMissingFunction;
    Stub->Link();
    OuterClass->AddFunctionToFunctionMap(Stub, Stub->GetFName());
    return Stub;
}

UObject* FLinkerLoad::IndexToObject(FPackageIndex Index) {
    if (Index.IsNull())
        return nullptr;
    if (Index.IsExport())
        return CreateExport(Index.ToExport());
    return ResolveObject(ResolveIndex(*Package, Index.Index));
}

UObject* FLinkerLoad::FindLoadedExport(const int32 Index) const {
    UObject* Object = ExportObjects[Index];
    if (Object != nullptr)
        WaitForLoad(Object);
    return Object;
}

FPackedObjectRef FLinkerLoad::PackObjectRef(FPackageIndex Index) const {
    const std::intptr_t ImportCount = static_cast<std::intptr_t>(Package->getHeader().getImportMap().size());
    if (Index.IsNull() || Index.Index > ExportObjects.Num() || -static_cast<std::intptr_t>(Index.Index) > ImportCount)
        return {};
    const std::uintptr_t Slot = static_cast<std::uintptr_t>(Index.Index + ImportCount);
    return FPackedObjectRef{((PackedRefBase + Slot) << 1) | 1};
}

namespace {

std::pair<FLinkerLoad*, FPackageIndex> UnpackObjectRef(FPackedObjectRef Ref) {
    const std::uintptr_t Slot = Ref.EncodedRef >> 1;
    const FPackedRefRanges& Ranges = GetPackedRefRanges();
    auto Found = std::upper_bound(Ranges.Bases.begin(), Ranges.Bases.end(), Slot);
    if (Found == Ranges.Bases.begin())
        return {nullptr, FPackageIndex()};
    const std::size_t LinkerIndex = static_cast<std::size_t>(Found - Ranges.Bases.begin()) - 1;
    FLinkerLoad* Linker = Ranges.Linkers[LinkerIndex];
    const std::intptr_t ImportCount = static_cast<std::intptr_t>(Linker->GetPackage().getHeader().getImportMap().size());
    const std::intptr_t Index = static_cast<std::intptr_t>(Slot - Ranges.Bases[LinkerIndex]) - ImportCount;
    return {Linker, FPackageIndex(static_cast<int32>(Index))};
}

}

UObject* ResolvePackedObjectRef(FPackedObjectRef Ref) {
    std::lock_guard Lock(GetLoaderMutex());
    const auto [Linker, Index] = UnpackObjectRef(Ref);
    if (Linker == nullptr) {
        ReportLoadProblem("load.packed_ref_stale", FString("a reference of a dropped linker"));
        return nullptr;
    }
    return Linker->IndexToObject(Index);
}

FString GetPackedObjectRefPathName(FPackedObjectRef Ref) {
    std::lock_guard Lock(GetLoaderMutex());
    const auto [Linker, Index] = UnpackObjectRef(Ref);
    if (Linker == nullptr || Index.IsNull())
        return FString();
    return FString(ResolveIndex(Linker->GetPackage(), Index.Index).getPathName());
}

FString FObjectPtr::GetPathName() const {
    const std::uintptr_t Value = std::atomic_ref<std::uintptr_t>(Handle).load(std::memory_order_acquire);
    if (Value & 1)
        return GetPackedObjectRefPathName(FPackedObjectRef{Value});
    const UObject* Object = reinterpret_cast<const UObject*>(Value);
    return Object == nullptr ? FString() : Object->GetPathName();
}

void FLinkerLoad::DetachExport(int32 Index) {
    std::lock_guard Lock(GetLoaderMutex());
    ExportObjects[Index] = nullptr;
    ExportFailed[Index] = 1;
}

int32 FLinkerLoad::FindExportIndex(const FString& ObjectPath) const {
    std::int64_t Index = Package->findExportByPath(ObjectPath.ToUtf8());
    return Index < 0 ? INDEX_NONE : static_cast<int32>(Index);
}

void FLinkerLoad::LoadAllObjects() {
    std::lock_guard Lock(GetLoaderMutex());
    FLoadScope Scope;
    for (int32 i = 0; i < ExportObjects.Num(); i++)
        CreateExport(i);
}

FExportArchive::FExportArchive(FLinkerLoad& InLinker, std::span<const std::byte> InBytes, int64 InCookedOffset, int64 InChunkOffset)
    : FMemoryReaderView(InBytes), Linker(InLinker), CookedOffset(InCookedOffset), ChunkOffset(InChunkOffset) {
    const ue5parse::Package& Package = InLinker.GetPackage();
    const ue5parse::FileProvider& Provider = Package.getProvider();
    SetUEVer(Package.getHeader().getFileVersionUE5());
    SetUseUnversionedPropertySerialization(Package.getHeader().isUnversioned());
    SetGame(Provider.getGame());
}

FArchive& FExportArchive::operator<<(FName& Value) {
    uint32 NameIndex = 0;
    uint32 Number = 0;
    Serialize(&NameIndex, sizeof(NameIndex));
    Serialize(&Number, sizeof(Number));
    if (IsError()) {
        Value = FName();
        return *this;
    }
    if (NameIndex >= Linker.GetNameCount()) {
        SetError(ErrorCode::InvalidData, "FExportArchive.FName");
        Value = FName();
        return *this;
    }
    Value = Linker.GetName(NameIndex, Number);
    return *this;
}

FArchive& FExportArchive::operator<<(UObject*& Value) {
    FPackageIndex Index = ReadPackageIndex();
    Value = IsError() ? nullptr : Linker.IndexToObject(Index);
    return *this;
}

FArchive& FExportArchive::operator<<(FObjectPtr& Value) {
    FPackageIndex Index = ReadPackageIndex();
    Value = IsError() ? FObjectPtr() : FObjectPtr(Linker.PackObjectRef(Index));
    return *this;
}

FArchive& FExportArchive::operator<<(FSoftObjectPath& Value) {
    Value.Reset();
    FName PackageName;
    *this << PackageName;
    FString SubPath;
    if (GetGame() >= EGame::GAME_UE5_1) {
        FName AssetName;
        *this << AssetName;
        Value.AssetPath.PackageName = PackageName;
        Value.AssetPath.AssetName = AssetName;
    } else {
        Value.SetPath(NameToString(PackageName));
    }
    if (GetGame() >= EGame::GAME_UE5_6)
        SubPath = ReadUtf8String();
    else
        *this << SubPath;
    if (GetGame() >= EGame::GAME_UE5_1 || !SubPath.IsEmpty())
        Value.SubPathString = std::move(SubPath);
    return *this;
}

FArchive& FExportArchive::operator<<(FSoftObjectPtr& Value) {
    Value.Reset();
    return *this << Value.ObjectID;
}

FArchive& FExportArchive::operator<<(FWeakObjectPtr& Value) {
    UObject* Object = nullptr;
    *this << Object;
    Value = Object;
    return *this;
}

}
