#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Field.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

struct FFieldPath {
    FFieldPath() = default;
    explicit FFieldPath(FField* Field) { *this = Field; }
    FFieldPath& operator=(FField* Field);

    FField* Get() const;
    FString ToString() const;
    void Generate(const FString& Path);
    void Reset() {
        ResolvedField = nullptr;
        ResolvedOwner.Reset();
        Path.Empty();
    }
    bool operator==(const FFieldPath& Other) const { return Get() == Other.Get(); }

    mutable FField* ResolvedField = nullptr;
    mutable FWeakObjectPtr ResolvedOwner;
    TArray<FName> Path;
};

}
