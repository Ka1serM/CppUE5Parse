#pragma once

#include <span>
#include <vector>

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

namespace ue5parse {
class Package;
class ResolvedObject;
enum class EIoChunkType : std::uint8_t;
}

namespace ue5parse::runtime {

// FPackageIndex: a reference in export data: an export (positive, index + 1), an import (negative) or null (UE's
// ObjectResource.h).
struct FPackageIndex {
    int32 Index = 0;

    constexpr FPackageIndex() = default;
    constexpr explicit FPackageIndex(int32 InIndex) : Index(InIndex) {}
    bool IsNull() const { return Index == 0; }
    bool IsImport() const { return Index < 0; }
    bool IsExport() const { return Index > 0; }
    int32 ToImport() const { return -Index - 1; }
    int32 ToExport() const { return Index - 1; }
    int32 ForDebugging() const { return Index; }
    bool operator==(const FPackageIndex&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FPackageIndex& Value) { return Ar << Value.Index; }

// FLinkerLoad: the runtime objects of one cooked package. It reads the Zen package itself: the header gives the
// names, imports and exports; CreateExport allocates an export's object, constructs it from its template, and
// serializes its cooked data through an FExportArchive (UE's FLinkerLoad and the Zen loader's FExportArchive).
class FLinkerLoad {
public:
    // The linker of a cooked package, made on first use. Thread-safe.
    static FLinkerLoad* FindOrCreate(const ue5parse::Package& Package);
    // The linker of a package path, loading the package's header; null when it does not exist.
    static FLinkerLoad* Load(const FString& PackageName);
    // Drops all linkers (at shutdown).
    static void ResetAll();

    explicit FLinkerLoad(const ue5parse::Package& InPackage);
    FLinkerLoad(const FLinkerLoad&) = delete;
    FLinkerLoad& operator=(const FLinkerLoad&) = delete;

    const ue5parse::Package& GetPackage() const { return *Package; }
    UPackage* GetLinkerRoot() const { return LinkerRoot; }
    int32 GetExportCount() const { return ExportObjects.Num(); }
    // The runtime object of an export, created and serialized on first use. Null when it cannot be built (its class
    // or data cannot be read): the reason is reported once.
    UObject* CreateExport(int32 Index);
    // The object of an export when it exists already; does not load.
    UObject* FindExport(int32 Index) const { return ExportObjects.IsValidIndex(Index) ? ExportObjects[Index] : nullptr; }
    // The objects of all exports that loaded.
    const TArray<UObject*>& GetExportObjects() const { return ExportObjects; }
    // The runtime object a resolved reference points to: an export of this or another package, or a native object.
    UObject* ResolveObject(const ue5parse::ResolvedObject& Object);
    // The runtime object of a reference in this package's export data.
    UObject* IndexToObject(FPackageIndex Index);
    // The template of an export (its export map entry's template), loading it.
    UObject* GetExportTemplate(int32 Index);
    int32 FindExportIndex(const FString& ObjectPath) const;
    // A name of the package's name map.
    FName GetName(uint32 NameIndex, uint32 Number) const;
    uint32 GetNameCount() const;
    // Creates every export (UE's LoadAllObjects).
    void LoadAllObjects();
    // Reads bulk data of this package (texture mips, mesh data): a chunk next to the export data.
    Result<std::vector<std::byte>> ReadBulkData(ue5parse::EIoChunkType Type, uint64 Offset, uint64 Size) const;

private:
    UObject* CreateExportInternal(int32 Index);
    // Creates the default subobject exports of the export.
    void CreateDefaultSubobjectExports(int32 OuterIndex);
    void SerializeExport(UObject* Object, int32 Index);

    const ue5parse::Package* Package;
    UPackage* LinkerRoot;
    TArray<UObject*> ExportObjects;
    TArray<uint8> ExportFailed;
    // The default subobject exports of each export of this package.
    TArray<TArray<int32>> DefaultSubobjectExports;
};

// The archive one export reads from: the export's bytes, names from its package's name map, object references
// through its linker (UE's FExportArchive of the Zen loader).
class FExportArchive : public FMemoryReaderView {
public:
    FExportArchive(FLinkerLoad& InLinker, std::span<const std::byte> InBytes, int64 InCookedOffset);

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override;
    FArchive& operator<<(UObject*& Value) override;
    FArchive& operator<<(FSoftObjectPath& Value) override;
    FArchive& operator<<(FSoftObjectPtr& Value) override;
    FArchive& operator<<(FWeakObjectPtr& Value) override;
    FLinkerLoad* GetLinker() override { return &Linker; }
    int64 GetCookedPosition() const override { return CookedOffset + Tell(); }

    // A reference as its raw index, without loading.
    FPackageIndex ReadPackageIndex() {
        FPackageIndex Index;
        *this << Index;
        return Index;
    }

private:
    FLinkerLoad& Linker;
    int64 CookedOffset;
};

// An export whose class is still serializing higher up the load stack (a dependency cycle) waits for the class to
// link, as UE's deferred dependency loads give such an export a placeholder. The object properties that read the export
// meanwhile record their value, and the reference is written once the export exists (UE's
// ULinkerPlaceholderExportObject and its referencing properties).
void BeginObjectReference();
void TrackDeferredExportReference(const class FObjectPropertyBase* Property, void* Value);
// The class has linked: the exports that waited for it are made and their references written.
void ResolveExportsWaitingForClass(const UClass* Class);

// Reports a load problem once per message (the code names the kind: "load.class_missing").
void ReportLoadProblem(const char* Code, const FString& Message);
using FLoadProblemHandler = void (*)(const char* Code, const FString& Message);
void SetLoadProblemHandler(FLoadProblemHandler Handler);

}
