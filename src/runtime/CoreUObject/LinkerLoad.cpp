#include <memory>
#include <mutex>
#include <set>
#include <string>

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

FLoadScope::FLoadScope() {
    if (GLoadScopeDepth++ == 0) GLoadedObjects = new TArray<UObject*>();
}

FLoadScope::~FLoadScope() {
    if (--GLoadScopeDepth != 0) return;
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

UObject* FLinkerLoad::CreateExport(int32 Index) {
    if (Index < 0 || Index >= ExportObjects.Num()) return nullptr;
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];
    if (ExportFailed[Index]) return nullptr;
    std::lock_guard Lock(GetLoaderMutex());
    if (ExportObjects[Index] != nullptr) return ExportObjects[Index];
    FLoadScope Scope;
    UObject* Object = CreateExportInternal(Index);
    if (Object == nullptr) ExportFailed[Index] = 1;
    return Object;
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
        ReportLoadProblem("load.class_unlinked", ExportPath);
        return nullptr;
    }
    if (Class->ClassConstructor == nullptr) {
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
        Class->ClassConstructor(FObjectInitializer(Object, Archetype, false, true));
    }
    Object->Linker = this;
    Object->LinkerIndex = Index;
    SerializeExport(Object, Index);
    Object->ClearFlags(RF_NeedLoad);
    FLoadScope::AddLoadedObject(Object);
    return Object;
}

void FLinkerLoad::SerializeExport(UObject* Object, int32 Index) {
    Result<std::span<const std::byte>> Bytes = Package->getExportBytes(static_cast<std::size_t>(Index));
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
    SetEngineVer(Provider.getEngine());
    SetUEVer(Package.getHeader().getFileVersionUE5());
    SetUseUnversionedPropertySerialization(Package.getHeader().isUnversioned());
    SetGameQuirks(Provider.getQuirks());
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
    if (EngineVer() >= EngineVersion::UE5_1) {
        // FTopLevelAssetPath: the package and the asset.
        FName AssetName;
        *this << AssetName;
        Value.AssetPath.PackageName = PackageName;
        Value.AssetPath.AssetName = AssetName;
    } else {
        // One name with both: "/Game/A/B.B".
        Value.SetPath(NameToString(PackageName));
    }
    if (EngineVer() >= EngineVersion::UE5_6) {
        SubPath = ReadUtf8String();
    } else {
        *this << SubPath;
    }
    if (EngineVer() >= EngineVersion::UE5_1 || !SubPath.IsEmpty()) Value.SubPathString = std::move(SubPath);
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
