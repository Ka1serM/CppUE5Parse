#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

struct FUniqueObjectGuid {
    FUniqueObjectGuid() = default;
    explicit FUniqueObjectGuid(const FGuid& InGuid) : Guid(InGuid) {}

    const FGuid& GetGuid() const { return Guid; }
    bool IsValid() const { return Guid != FGuid(); }
    void Reset() { Guid = FGuid(); }
    bool operator==(const FUniqueObjectGuid& Other) const { return Guid == Other.Guid; }
    friend uint32 GetTypeHash(const FUniqueObjectGuid& Id) { return GetTypeHash(Id.Guid); }
    friend FArchive& operator<<(FArchive& Ar, FUniqueObjectGuid& Id) { return Ar << Id.Guid; }

private:
    FGuid Guid;
};

//! Cooked packages have no GUID annotations, so a lazy pointer only resolves an object that was assigned to it.
struct FLazyObjectPtr {
    FLazyObjectPtr() = default;
    FLazyObjectPtr(const UObject* Object) : WeakPtr(Object) {}

    UObject* Get() const { return WeakPtr.Get(); }
    const FUniqueObjectGuid& GetUniqueID() const { return ObjectID; }
    bool IsNull() const { return !ObjectID.IsValid() && WeakPtr.Get() == nullptr; }
    bool IsValid() const { return Get() != nullptr; }
    bool IsPending() const { return Get() == nullptr && ObjectID.IsValid(); }
    void Reset() {
        WeakPtr.Reset();
        TagAtLastTest = 0;
        ObjectID.Reset();
    }
    bool operator==(const FLazyObjectPtr& Other) const { return ObjectID == Other.ObjectID && WeakPtr == Other.WeakPtr; }
    friend uint32 GetTypeHash(const FLazyObjectPtr& Pointer) { return GetTypeHash(Pointer.ObjectID); }
    friend FArchive& operator<<(FArchive& Ar, FLazyObjectPtr& Pointer) {
        Pointer.WeakPtr.Reset();
        Pointer.TagAtLastTest = 0;
        return Ar << Pointer.ObjectID;
    }

    mutable FWeakObjectPtr WeakPtr;
    mutable int32 TagAtLastTest = 0;
    FUniqueObjectGuid ObjectID;
};

template <class T>
class TLazyObjectPtr {
public:
    TLazyObjectPtr() = default;
    TLazyObjectPtr(const T* Object) : LazyPtr(reinterpret_cast<const UObject*>(Object)) {}

    T* Get() const { return reinterpret_cast<T*>(LazyPtr.Get()); }
    T* operator->() const { return Get(); }
    const FUniqueObjectGuid& GetUniqueID() const { return LazyPtr.GetUniqueID(); }
    bool IsNull() const { return LazyPtr.IsNull(); }
    bool IsValid() const { return LazyPtr.IsValid(); }
    bool IsPending() const { return LazyPtr.IsPending(); }
    void Reset() { LazyPtr.Reset(); }
    bool operator==(const TLazyObjectPtr& Other) const { return LazyPtr == Other.LazyPtr; }
    friend uint32 GetTypeHash(const TLazyObjectPtr& Pointer) { return GetTypeHash(Pointer.LazyPtr); }
    friend FArchive& operator<<(FArchive& Ar, TLazyObjectPtr& Pointer) { return Ar << Pointer.LazyPtr; }

private:
    FLazyObjectPtr LazyPtr;
};

}
