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
    friend uint32 GetTypeHash(const FFieldPath& FieldPath) { return static_cast<uint32>(reinterpret_cast<uintptr_t>(FieldPath.Get()) >> 4); }

    mutable FField* ResolvedField = nullptr;
    mutable FWeakObjectPtr ResolvedOwner;
    TArray<FName> Path;
};

template <class PropertyType>
struct TFieldPath : FFieldPath {
    TFieldPath() = default;
    TFieldPath(PropertyType* Property) : FFieldPath(Property) {}

    PropertyType* Get() const { return static_cast<PropertyType*>(FFieldPath::Get()); }
    PropertyType* operator->() const { return Get(); }
};

static_assert(sizeof(TFieldPath<FField>) == sizeof(FFieldPath));

}
