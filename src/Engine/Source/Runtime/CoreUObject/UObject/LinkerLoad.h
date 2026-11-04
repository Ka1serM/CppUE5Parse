#pragma once

#include <span>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse {

class Package;
class ResolvedObject;
enum class EIoChunkType : std::uint8_t;

}

namespace ue5parse::runtime {

struct FPackageIndex {
    int32 Index = 0;

    constexpr FPackageIndex() = default;
    constexpr explicit FPackageIndex(int32 InIndex) : Index(InIndex) {}
    bool IsNull() const { return Index == 0; }
    bool IsExport() const { return Index > 0; }
    int32 ToExport() const { return Index - 1; }
    bool operator==(const FPackageIndex&) const = default;
};

inline FArchive& operator<<(FArchive& Ar, FPackageIndex& Value) { return Ar << Value.Index; }

class FLinkerLoad {
public:
    static FLinkerLoad* FindOrCreate(const ue5parse::Package& Package);
    static FLinkerLoad* Load(const FString& PackageName);
    static void ResetAll();

    explicit FLinkerLoad(const ue5parse::Package& InPackage);
    FLinkerLoad(const FLinkerLoad&) = delete;
    FLinkerLoad& operator=(const FLinkerLoad&) = delete;

    const ue5parse::Package& GetPackage() const { return *Package; }
    UPackage* GetLinkerRoot() const { return LinkerRoot; }
    UObject* CreateExport(int32 Index);
    UObject* ResolveObject(const ue5parse::ResolvedObject& Object);
    UObject* IndexToObject(FPackageIndex Index);
    UObject* GetExportTemplate(int32 Index);
    int32 FindExportIndex(const FString& ObjectPath) const;
    FName GetName(uint32 NameIndex, uint32 Number) const;
    uint32 GetNameCount() const;
    void LoadAllObjects();
    FPackedObjectRef PackObjectRef(FPackageIndex Index) const;
    void DetachExport(int32 Index);

private:
    UObject* CreateExportInternal(int32 Index);
    void CreateDefaultSubobjectExports(int32 OuterIndex);
    void SerializeExport(UObject* Object, int32 Index, const Result<std::vector<std::byte>>& Bytes);
    UObject* FindLoadedExport(int32 Index) const;

    const ue5parse::Package* Package;
    UPackage* LinkerRoot;
    std::uintptr_t PackedRefBase = 0;
    TArray<UObject*> ExportObjects;
    TArray<uint8> ExportFailed;
    TArray<TArray<int32>> DefaultSubobjectExports;
};

class FExportArchive : public FMemoryReaderView {
public:
    FExportArchive(FLinkerLoad& InLinker, std::span<const std::byte> InBytes, int64 InCookedOffset, int64 InChunkOffset);

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override;
    FArchive& operator<<(UObject*& Value) override;
    FArchive& operator<<(FObjectPtr& Value) override;
    FArchive& operator<<(FSoftObjectPath& Value) override;
    FArchive& operator<<(FSoftObjectPtr& Value) override;
    FArchive& operator<<(FWeakObjectPtr& Value) override;
    FLinkerLoad* GetLinker() override { return &Linker; }
    int64 GetCookedPosition() const override { return CookedOffset + Tell(); }
    int64 GetExportChunkPosition() const override { return ChunkOffset < 0 ? -1 : ChunkOffset + Tell(); }

    FPackageIndex ReadPackageIndex() {
        FPackageIndex Index;
        *this << Index;
        return Index;
    }

private:
    FLinkerLoad& Linker;
    int64 CookedOffset;
    int64 ChunkOffset;
};

void BeginObjectReference();
void TrackDeferredExportReference(const class FObjectPropertyBase* Property, void* Value);
void ResolveExportsWaitingForClass(const UClass* Class);

void ReportLoadProblem(const char* Code, const FString& Message);
using FLoadProblemHandler = void (*)(const char* Code, const FString& Message);
void SetLoadProblemHandler(FLoadProblemHandler Handler);

}
