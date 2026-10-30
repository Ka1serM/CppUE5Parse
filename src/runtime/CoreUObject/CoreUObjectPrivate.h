#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse {
struct PropertyType;
}

// Helpers shared by the CoreUObject sources; not part of the public API.
namespace ue5parse::runtime {

// A runtime property from a mappings type tree (native classes and structs).
FProperty* CreatePropertyFromMappings(FName Name, const ue5parse::PropertyType& Type, int32 ArrayDim, FFieldVariant Owner);

// StaticAllocateObject's identity handoff and registration for memory the caller allocated (the native type objects,
// whose metaclass may still be unlinked during the bootstrap).
void SetPendingIdentity(UObjectBase* Object, UClass* Class, UObject* Outer, FName Name, EObjectFlags Flags);

// The global lock of loading and class building. Recursive: loads nest.
std::recursive_mutex& GetLoaderMutex();

// UObject* <-> 8-byte script pointer slots.
inline ScriptPointerType ToScriptPointer(const void* Pointer) { return static_cast<ScriptPointerType>(reinterpret_cast<uintptr_t>(Pointer)); }

// Tracks the objects loaded during the outermost load, so that PostLoad runs once the whole load finished (UE's
// EndLoad).
class FLoadScope {
public:
    FLoadScope();
    ~FLoadScope();
    FLoadScope(const FLoadScope&) = delete;
    FLoadScope& operator=(const FLoadScope&) = delete;
    static void AddLoadedObject(UObject* Object);
};

}
