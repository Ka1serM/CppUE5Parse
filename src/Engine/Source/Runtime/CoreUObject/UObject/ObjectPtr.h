#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class UObject;

struct FPackedObjectRef {
    std::uintptr_t EncodedRef = 0;
};

UObject* ResolvePackedObjectRef(FPackedObjectRef Ref);
FString GetPackedObjectRefPathName(FPackedObjectRef Ref);

//! Native code that reads a reference as a plain pointer must read it through Get().
class FObjectPtr {
public:
    FObjectPtr() = default;
    FObjectPtr(std::nullptr_t) {}
    FObjectPtr(UObject* Object) : Handle(reinterpret_cast<std::uintptr_t>(Object)) {}
    explicit FObjectPtr(FPackedObjectRef Ref) : Handle(Ref.EncodedRef) {}

    UObject* Get() const {
        std::atomic_ref<std::uintptr_t> Bits(Handle);
        std::uintptr_t Value = Bits.load(std::memory_order_acquire);
        if (Value & 1) {
            Value = reinterpret_cast<std::uintptr_t>(ResolvePackedObjectRef(FPackedObjectRef{Value}));
            Bits.store(Value, std::memory_order_release);
        }
        return reinterpret_cast<UObject*>(Value);
    }
    bool IsResolved() const { return (std::atomic_ref<std::uintptr_t>(Handle).load(std::memory_order_acquire) & 1) == 0; }
    FString GetPathName() const;
    bool operator==(const FObjectPtr& Other) const { return Get() == Other.Get(); }

private:
    mutable std::uintptr_t Handle = 0;
};

static_assert(sizeof(FObjectPtr) == sizeof(UObject*));
static_assert(std::atomic_ref<std::uintptr_t>::required_alignment == alignof(std::uintptr_t));

template <class T>
class TObjectPtr {
public:
    TObjectPtr() = default;
    TObjectPtr(std::nullptr_t) {}
    TObjectPtr(T* Object) : Ptr(const_cast<UObject*>(reinterpret_cast<const UObject*>(Object))) {}
    template <class U>
    TObjectPtr(const TObjectPtr<U>& Other) : TObjectPtr(Other.Get()) {}

    T* Get() const { return reinterpret_cast<T*>(Ptr.Get()); }
    operator T*() const { return Get(); }
    T* operator->() const { return Get(); }
    T& operator*() const { return *Get(); }
    friend FArchive& operator<<(FArchive& Ar, TObjectPtr& Value) { return Ar << Value.Ptr; }

private:
    FObjectPtr Ptr;
};

static_assert(sizeof(TObjectPtr<UObject>) == sizeof(UObject*));

}
