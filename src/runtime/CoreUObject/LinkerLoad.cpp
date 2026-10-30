#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <algorithm>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "CoreUObjectPrivate.h"

#include "ue5parse/core/Result.h"
#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

ankerl::unordered_dense::map<const ue5parse::Package*, std::unique_ptr<FLinkerLoad>>& GetLinkers() {
    static ankerl::unordered_dense::map<const ue5parse::Package*, std::unique_ptr<FLinkerLoad>> Linkers;
    return Linkers;
}

FLoadProblemHandler GLoadProblemHandler = nullptr;

// The flags of a cooked export that a loaded object keeps.
constexpr uint32 LoadFlagsMask = RF_Public | RF_Standalone | RF_Transactional | RF_ClassDefaultObject | RF_ArchetypeObject |
                                 RF_Transient | RF_DefaultSubObject | RF_InheritableComponentTemplate;

thread_local int32 GLoadScopeDepth = 0;
thread_local TArray<UObject*>* GLoadedObjects = nullptr;

// The thunk of a native function the runtime does not have: evaluates the call's parameters (so the bytecode stays in
// step), reports the function once, and leaves the result alone.
void ExecMissingFunction(UObject* Context, FFrame& Stack, RESULT_DECL) {
    alignas(16) uint8 Scratch[1024];
    while (Stack.Code != nullptr && *Stack.Code != EX_EndFunctionParms) {
        std::memset(Scratch, 0, sizeof(Scratch));
        Stack.MostRecentPropertyAddress = nullptr;
        Stack.Step(Stack.Object, Scratch);
    }
    if (Stack.Code != nullptr) Stack.Code++;
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
        if (!Reported.insert(Key).second) return;
    }
    if (GLoadProblemHandler != nullptr) GLoadProblemHandler(Code, Message);
}

void SetLoadProblemHandler(FLoadProblemHandler Handler) { GLoadProblemHandler = Handler; }

namespace {

struct FDeferredExport {
    FLinkerLoad* Linker;
    int32 Index;
    const UClass* Class;
    std::vector<std::pair<const FObjectPropertyBase*, void*>> References;
};

// The exports that wait for their class to link, and the export that the current object reference resolved to.
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
    if (GLoadScopeDepth++ == 0) GLoadedObjects = new TArray<UObject*>();
}

FLoadScope::~FLoadScope() {
    if (--GLoadScopeDepth != 0) return;
    // An export whose class did not link by the end of the outermost load stays missing.
    for (const FDeferredExport& Deferred : GDeferredExports) {
        const UClass* Unlinked = Deferred.Class;
        while (Unlinked->GetSuperClass() != nullptr && !Unlinked->GetSuperClass()->IsLinked()) Unlinked = Unlinked->GetSuperClass();
        ReportLoadProblem("load.class_unlinked", FString(Deferred.Linker->GetPackage().getExportPathName(static_cast<std::size_t>(Deferred.Index))) +
                                                     FString(" (") + Unlinked->GetPathName() + FString(")"));
    }
    // PostLoad runs once everything of the outermost load is serialized; objects loaded by a PostLoad join the list.
    TArray<UObject*>* Loaded = GLoadedObjects;
    GLoadScopeDepth++;
    for (int32 i = 0; i < Loaded->Num(); i++) {
        UObject* Object = (*Loaded)[i];
        Object->PostLoad();
        Object->SetFlags(RF_LoadCompleted);
    }
    GLoadScopeDepth--;
    GLoadedObjects = nullptr;
    delete Loaded;
}

void FLoadScope::AddLoadedObject(UObject* Object) {
    if (GLoadedObjects != nullptr) {
        GLoadedObjects->Add(Object);
    } else {
        Object->PostLoad();
        Object->SetFlags(RF_LoadCompleted);
    }
}

FLinkerLoad* FLinkerLoad::FindOrCreate(const ue5parse::Package& Package) {
    std::lock_guard Lock(GetLoaderMutex());
    auto& Linkers = GetLinkers();
    auto Found = Linkers.find(&Package);
    if (Found != Linkers.end()) return Found->second.get();
    auto Linker = std::make_unique<FLinkerLoad>(Package);
    FLinkerLoad* Result = Linker.get();
    Linkers.emplace(&Package, std::move(Linker));
    return Result;
}

FLinkerLoad* FLinkerLoad::Load(const FString& PackageName) {
    Result<const ue5parse::Package*> Package = GetRuntimeProvider().loadPackage(PackageName.ToUtf8());
    if (!Package) return nullptr;
    return FindOrCreate(**Package);
}

void FLinkerLoad::ResetAll() {
    std::lock_guard Lock(GetLoaderMutex());
    GetLinkers().clear();
}

FLinkerLoad::FLinkerLoad(const ue5parse::Package& InPackage) : Package(&InPackage) {
    LinkerRoot = CreatePackage(FString(Package->getName().toString()));
    int32 Count = static_cast<int32>(Package->getExportCount());
    ExportObjects.SetNum(Count);
    ExportFailed.SetNum(Count);
    DefaultSubobjectExports.SetNum(Count);
    for (int32 i = 0; i < Count; i++) {
        const ue5parse::FExportMapEntry& Entry = Package->getExportMapEntry(static_cast<std::size_t>(i));
        if (!(Entry.objectFlags & RF_DefaultSubObject) || Entry.outerIndex.isNull()) continue;
        const ue5parse::ResolvedObject Outer = Package->resolve(Entry.outerIndex);
        if (!Outer.isExport() || Outer.getPackage() != Package) continue;
        const int32 OuterIndex = static_cast<int32>(Outer.getExportIndex());
        if (OuterIndex >= 0 && OuterIndex < Count) DefaultSubobjectExports[OuterIndex].Add(i);
    }
}

FName FLinkerLoad::GetName(uint32 NameIndex, uint32 Number) const {
    std::span<const FName> Names = Package->getHeader().getNameMap();
    if (NameIndex >= Names.size()) return FName();
    const FName& Base = Names[NameIndex];
    return FName(Base.getText(), Base.getHash(), Number);
}

uint32 FLinkerLoad::GetNameCount() const { return static_cast<uint32>(Package->getHeader().getNameMap().size()); }

UObject* FLinkerLoad::GetExportTemplate(int32 Index) {
    if (Index < 0 || Index >= ExportObjects.Num()) return nullptr;
    const ue5parse::FExportMapEntry& Entry = Package->getExportMapEntry(static_cast<std::size_t>(Index));
    if (Entry.templateIndex.isNull()) return nullptr;
    return ResolveObject(Package->resolve(Entry.templateIndex));
}

Result<std::vector<std::byte>> FLinkerLoad::ReadBulkData(ue5parse::EIoChunkType Type, uint64 Offset, uint64 Size) const {
    return Package->readBulkData(Type, Offset, Size);
}

void BeginObjectReference() { GLastDeferredExport = INDEX_NONE; }

void TrackDeferredExportReference(const FObjectPropertyBase* Property, void* Value) {
    if (GLastDeferredExport == INDEX_NONE) return;
    GDeferredExports[static_cast<std::size_t>(GLastDeferredExport)].References.emplace_back(Property, Value);
    GLastDeferredExport = INDEX_NONE;
}

void ResolveExportsWaitingForClass(const UClass* Class) {
    std::lock_guard Lock(GetLoaderMutex());
    for (std::size_t i = 0; i < GDeferredExports.size();) {
        if (GDeferredExports[i].Class != Class) {
            ++i;
            continue;
        }
        FDeferredExport Deferred = std::move(GDeferredExports[i]);
        GDeferredExports.erase(GDeferredExports.begin() + static_cast<std::ptrdiff_t>(i));
        UObject* Object = Deferred.Linker->CreateExport(Deferred.Index);
        if (Object == nullptr) continue;
        // A reference that something else wrote since keeps its value.
        for (const auto& [Property, Value] : Deferred.References)
            if (Property->GetObjectPropertyValue(Value) == nullptr) Property->SetObjectPropertyValue(Value, Object);
    }
}

UObject* FLinkerLoad::CreateExport(int32 Index) {
    // Only a deferred export leaves the signal for the reference that asked for it.
    GLastDeferredExport = INDEX_NONE;
    if (Index < 0 || Index >= ExportObjects.Num()) return nullptr;
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];
    if (ExportFailed[Index]) return nullptr;
    std::lock_guard Lock(GetLoaderMutex());
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];
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
    if (Object != nullptr) return Object;
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
    UClass* Class = Cast<UClass>(ResolveObject(Package->resolve(Entry.classIndex)));
    if (Class == nullptr) {
        ReportLoadProblem("load.class_missing", ExportPath + FString(" (") + FString(Package->resolve(Entry.classIndex).getPathName()) + FString(")"));
        return nullptr;
    }
    if (!Class->IsLinked()) {
        // The class is still serializing higher up the load stack: the export waits for it.
        GDeferredExports.push_back(FDeferredExport{this, Index, Class, {}});
        return nullptr;
    }
    const FClassConstructorType Constructor = Class->ClassConstructor;
    if (Constructor == nullptr) {
        ReportLoadProblem("load.class_constructor_missing", ExportPath + FString(" (") + Class->GetPathName() + FString(")"));
        return nullptr;
    }
    UObject* Outer = Entry.outerIndex.isNull() ? LinkerRoot : ResolveObject(Package->resolve(Entry.outerIndex));
    if (Outer == nullptr) {
        ReportLoadProblem("load.outer_missing", ExportPath);
        return nullptr;
    }
    // Loading the outer may have made this object already (a default subobject the outer's constructor made, which
    // the outer's linker found here).
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];
    UObject* Template = Entry.templateIndex.isNull() ? nullptr : ResolveObject(Package->resolve(Entry.templateIndex));
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];

    // Resolve defaults before allocation: a nested load must not replace this object's pending identity.
    UObject* Archetype = Template;
    if (Archetype == nullptr) Archetype = (Entry.objectFlags & RF_ClassDefaultObject)
                                                   ? (Class->GetSuperClass() ? Class->GetSuperClass()->GetDefaultObject() : nullptr)
                                                   : Class->GetDefaultObject();
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];

    FName Name = Package->getExportName(static_cast<std::size_t>(Index));
    EObjectFlags Flags = EObjectFlags(Entry.objectFlags & LoadFlagsMask);
    UObject* Object = StaticFindObjectFast(nullptr, Outer, Name);
    if (Object != nullptr && !Object->IsA(Class)) {
        ReportLoadProblem("load.subobject_class", ExportPath);
        return nullptr;
    }
    if (Object != nullptr) {
        // A subobject the outer's C++ constructor made: the cooked data goes into it.
        Object->SetFlags(Flags | RF_NeedLoad | RF_WasLoaded);
        ExportObjects[Index] = Object;
    } else {
        Object = StaticAllocateObject(Class, Outer, Name, Flags | RF_NeedLoad | RF_WasLoaded);
        ExportObjects[Index] = Object;
        if (Flags & RF_ClassDefaultObject) Class->ClassDefaultObject = Object;
        Constructor(FObjectInitializer(Object, Archetype, false, true));
    }
    Object->Linker = this;
    Object->LinkerIndex = Index;
    SerializeExport(Object, Index);
    Object->ClearFlags(RF_NeedLoad);
    FLoadScope::AddLoadedObject(Object);
    // UE loads every export of a package, so an object has each of its default subobjects, also one that no property
    // names (a component that a game constructor made, such as an effect manager): an archetype for the instances that
    // copy it, a placed actor for the replication that names it.
    CreateDefaultSubobjectExports(Index);
    return Object;
}

void FLinkerLoad::CreateDefaultSubobjectExports(const int32 OuterIndex) {
    for (const int32 Subobject : DefaultSubobjectExports[OuterIndex]) CreateExport(Subobject);
}

void FLinkerLoad::SerializeExport(UObject* Object, int32 Index) {
    // The bytes are freed at the end of this function: a loaded object keeps no view into them.
    Result<std::vector<std::byte>> Bytes = Package->readExportBytes(static_cast<std::size_t>(Index));
    if (!Bytes) {
        ReportLoadProblem("load.export_unreadable", Object->GetPathName());
        return;
    }
    FExportArchive Ar(*this, *Bytes, static_cast<int64>(Package->getHeader().getExportCookedOffset(static_cast<std::size_t>(Index))));
    // A class default object reads only its properties (and sparse class data): UE's SerializeDefaultObject.
    if (Object->HasAnyFlags(RF_ClassDefaultObject)) {
        Object->GetClass()->SerializeDefaultObject(Object, Ar);
    } else {
        Object->Serialize(Ar);
    }
    if (Ar.IsError()) {
        const ue5parse::Error& Error = Ar.GetError();
        ReportLoadProblem("load.serialize", Object->GetFullName() + FString(" (") + FString(ue5parse::getErrorName(Error.code)) +
                                                 FString(" at ") + FString(Error.where) + FString(")"));
    } else if (!Ar.AtEnd()) {
        ReportLoadProblem("load.unread_bytes", Object->GetClass()->GetName());
    }
}

UObject* FLinkerLoad::ResolveObject(const ue5parse::ResolvedObject& Resolved) {
    if (Resolved.isNull()) return nullptr;
    if (Resolved.isExport()) {
        FLinkerLoad* Linker = FindOrCreate(*Resolved.getPackage());
        return Linker->CreateExport(static_cast<int32>(Resolved.getExportIndex()));
    }
    // A script object: "/Script/Engine" > "Actor" > "K2_GetActorLocation", or "Default__Actor".
    ue5parse::ResolvedObject Outer = Resolved.getOuter();
    FName Name = Resolved.getName();
    if (Outer.isNull()) return CreatePackage(FString(Name.toString()));
    ue5parse::ResolvedObject OuterOuter = Outer.getOuter();
    if (OuterOuter.isNull()) {
        if (UClass* Class = FindNativeClass(Name)) return Class;
        if (UScriptStruct* Struct = FindNativeStruct(Name)) return Struct;
        if (UEnum* Enum = FindNativeEnum(Name)) return Enum;
        if (UFunction* Delegate = FindNativeDelegateFunction(Name)) return Delegate;
        std::string_view Text = Name.getText();
        if (Text.starts_with("Default__"))
            if (UClass* Class = FindNativeClass(FName(Text.substr(9)))) return Class->GetDefaultObject();
        ReportLoadProblem("load.native_missing", FString(Resolved.getPathName()));
        return nullptr;
    }
    UObject* OuterObject = ResolveObject(Outer);
    UClass* OuterClass = Cast<UClass>(OuterObject);
    if (OuterClass == nullptr) {
        if (OuterObject != nullptr)
            if (UObject* Child = StaticFindObjectFast(nullptr, OuterObject, Name)) return Child;
        ReportLoadProblem("load.native_missing", FString(Resolved.getPathName()));
        return nullptr;
    }
    if (UFunction* Function = OuterClass->FindFunctionByName(Name, false)) return Function;
    if (UObject* Child = StaticFindObjectFast(nullptr, OuterClass, Name)) return Child;
    // A native function the runtime has no thunk for: a stub that reports it when called.
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
    if (Index.IsNull()) return nullptr;
    if (Index.IsExport()) return CreateExport(Index.ToExport());
    return ResolveObject(Package->resolve(Index.Index));
}

int32 FLinkerLoad::FindExportIndex(const FString& ObjectPath) const {
    std::int64_t Index = Package->findExportByPath(ObjectPath.ToUtf8());
    return Index < 0 ? INDEX_NONE : static_cast<int32>(Index);
}

void FLinkerLoad::LoadAllObjects() {
    FLoadScope Scope;
    for (int32 i = 0; i < ExportObjects.Num(); i++) CreateExport(i);
}

FExportArchive::FExportArchive(FLinkerLoad& InLinker, std::span<const std::byte> InBytes, int64 InCookedOffset)
    : FMemoryReaderView(InBytes), Linker(InLinker), CookedOffset(InCookedOffset) {
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

FArchive& FExportArchive::operator<<(FSoftObjectPath& Value) {
    Value.Reset();
    FName PackageName;
    *this << PackageName;
    FString SubPath;
    if (GetGame() >= EGame::GAME_UE5_1) {
        // FTopLevelAssetPath: the package and the asset.
        FName AssetName;
        *this << AssetName;
        Value.AssetPath.PackageName = PackageName;
        Value.AssetPath.AssetName = AssetName;
    } else {
        // One name with both: "/Game/A/B.B".
        Value.SetPath(NameToString(PackageName));
    }
    if (GetGame() >= EGame::GAME_UE5_6) {
        SubPath = ReadUtf8String();
    } else {
        *this << SubPath;
    }
    if (GetGame() >= EGame::GAME_UE5_1 || !SubPath.IsEmpty()) Value.SubPathString = std::move(SubPath);
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
