#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Field.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

// FFieldPath: a field by its path from its owning struct, resolved on first use.
struct FFieldPath {
    FFieldPath() = default;
    explicit FFieldPath(FField* Field) { *this = Field; }
    FFieldPath& operator=(FField* Field);

    FField* Get() const;
    // "/Script/Engine.Actor:RootComponent" and nested inner names after the owner, as UE's ToString.
    FString ToString() const;
    // Parses a path that ToString made.
    void Generate(const FString& Path);
    void Reset() {
        ResolvedField = nullptr;
        ResolvedOwner.Reset();
        Path.Empty();
    }
    bool operator==(const FFieldPath& Other) const { return Get() == Other.Get(); }

    mutable FField* ResolvedField = nullptr;
    mutable FWeakObjectPtr ResolvedOwner;
    // Innermost name first, then outer field names, then the owner's path names, as UE stores it.
    TArray<FName> Path;
};

}
