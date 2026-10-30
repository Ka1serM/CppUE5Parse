#pragma once

#include <span>

#include "ue5parse/runtime/CoreUObject/UObject/FieldPath.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "PropertyAccess.generated.h"

namespace ue5parse::runtime {

// UE's EPropertyAccessSegmentFlags: what a path segment is, resolved at compile time.
enum class EPropertyAccessSegmentFlags : uint16 {
    Unresolved = 0,
    Struct,
    Leaf,
    Object,
    WeakObject,
    SoftObject,
    Array,
    ArrayOfStructs,
    ArrayOfObjects,
    Function = (1 << 15),
    ModifierFlags = Function,
};

USTRUCT()
struct FPropertyAccessSegment {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    UStruct* Struct = nullptr;
    // The property that the name resolved to at compile time; for a function, its return property.
    UPROPERTY()
    FFieldPath Property;
    UPROPERTY()
    UFunction* Function = nullptr;
    UPROPERTY()
    int32 ArrayIndex = INDEX_NONE;
    UPROPERTY()
    uint16 Flags = 0;
};

USTRUCT()
struct FPropertyAccessPath {
    GENERATED_BODY()
    UPROPERTY()
    int32 PathSegmentStartIndex = INDEX_NONE;
    UPROPERTY()
    int32 PathSegmentCount = INDEX_NONE;
};

enum class EPropertyAccessCopyType : uint8 {
    None,
    Plain,
    Complex,
    Bool,
    Struct,
    Object,
    Name,
    Array,
    PromoteBoolToByte,
    PromoteBoolToInt32,
    PromoteBoolToInt64,
    PromoteBoolToFloat,
    PromoteBoolToDouble,
    PromoteByteToInt32,
    PromoteByteToInt64,
    PromoteByteToFloat,
    PromoteByteToDouble,
    PromoteInt32ToInt64,
    PromoteInt32ToFloat,
    PromoteInt32ToDouble,
    PromoteFloatToDouble,
    DemoteDoubleToFloat,
    PromoteArrayFloatToDouble,
    DemoteArrayDoubleToFloat,
    PromoteMapValueFloatToDouble,
    DemoteMapValueDoubleToFloat,
};

USTRUCT()
struct FPropertyAccessCopy {
    GENERATED_BODY()
    UPROPERTY()
    int32 AccessIndex = INDEX_NONE;
    UPROPERTY()
    int32 DestAccessStartIndex = INDEX_NONE;
    UPROPERTY()
    int32 DestAccessEndIndex = INDEX_NONE;
    UPROPERTY()
    EPropertyAccessCopyType Type = EPropertyAccessCopyType::Plain;
};

USTRUCT()
struct FPropertyAccessCopyBatch {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPropertyAccessCopy> Copies;
};

// UE's EAnimPropertyAccessCallSite: the index of a copy batch. The exposed value handlers use the unbatched copies.
namespace EAnimPropertyAccessCallSite {
enum Type : int32 {
    WorkerThread_Unbatched,
    WorkerThread_Batched_PreEventGraph,
    WorkerThread_Batched_PostEventGraph,
    GameThread_Batched_PreEventGraph,
    GameThread_Batched_PostEventGraph,
};
}

enum class EPropertyAccessIndirectionType : uint8 {
    Offset,
    Object,
    Array,
    ScriptFunction,
    NativeFunction,
};

enum class EPropertyAccessObjectType : uint8 {
    None,
    Object,
    WeakObject,
    SoftObject,
};

// One step of a resolved path (UE's PropertyAccess.h). The library makes them on load; they are not cooked.
struct FPropertyAccessIndirection {
    FProperty* Property = nullptr;
    UFunction* Function = nullptr;
    int32 ReturnBufferSize = 0;
    int32 ReturnBufferAlignment = 0;
    int32 ArrayIndex = INDEX_NONE;
    uint32 Offset = 0;
    EPropertyAccessObjectType ObjectType = EPropertyAccessObjectType::None;
    EPropertyAccessIndirectionType Type = EPropertyAccessIndirectionType::Offset;
};

// The indirections of one path and its leaf property; a path that does not resolve has no leaf.
struct FPropertyAccessIndirectionChain {
    FProperty* Property = nullptr;
    int32 IndirectionStartIndex = INDEX_NONE;
    int32 IndirectionEndIndex = INDEX_NONE;
};

USTRUCT()
struct FPropertyAccessLibrary {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FPropertyAccessSegment> PathSegments;
    UPROPERTY()
    TArray<FPropertyAccessPath> SrcPaths;
    UPROPERTY()
    TArray<FPropertyAccessPath> DestPaths;
    UPROPERTY()
    TArray<FPropertyAccessCopyBatch> CopyBatchArray;

    // UE's PropertyAccess::PatchPropertyOffsets: resolves the paths' segments into indirections on load.
    void PatchPropertyOffsets();
    // UE's PropertyAccess::ProcessCopy. The post copy operation, when there is one, runs on each destination after its
    // copy.
    using FPostCopyOperation = void (*)(const FProperty* Property, void* Address);
    void ProcessCopy(UObject* Object, int32 BatchId, int32 CopyIndex, FPostCopyOperation PostCopyOperation = nullptr) const;
    // UE's PropertyAccess::ProcessCopies: every copy of a batch, in order.
    void ProcessCopies(UObject* Object, int32 BatchId) const;

    // All source accesses and all destination accesses, one to one with the paths.
    TArray<FPropertyAccessIndirectionChain> SrcAccesses;
    TArray<FPropertyAccessIndirectionChain> DestAccesses;
    TArray<FPropertyAccessIndirection> Indirections;
    bool bHasBeenPostLoaded = false;

private:
    bool ResolveIndirectionsOnLoad(std::span<const FPropertyAccessSegment> Segments, FPropertyAccessIndirectionChain& Access);
    template <class AddressFunctionType>
    void GetAccessAddress(void* Container, const FPropertyAccessIndirectionChain& Access, AddressFunctionType&& AddressFunction) const;
    static void PerformCopy(const FPropertyAccessCopy& Copy, const FProperty* SrcProperty, const void* SrcAddr, const FProperty* DestProperty, void* DestAddr,
                            FPostCopyOperation PostCopyOperation);
};

}
