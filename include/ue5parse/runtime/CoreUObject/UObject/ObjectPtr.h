#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class UObject;

// FPackedObjectRef: an unresolved object reference, as the linker of the package that holds it encodes it. The low bit
// is set, which no object address has.
struct FPackedObjectRef {
    std::uintptr_t EncodedRef = 0;
};

// The object that a packed reference names, loaded on this call. Null when it cannot be loaded (the reason is reported
// once). Thread-safe.
UObject* ResolvePackedObjectRef(FPackedObjectRef Ref);

// FObjectPtr: an object reference that loads its object on the first read (UE's FObjectPtr with late resolve). It has
// the size and the bits of a UObject*: the object's address, or a packed reference that the first read resolves and
// replaces with the address. Native code that reads a reference as a plain pointer must read it through Get().
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
    bool operator==(const FObjectPtr& Other) const { return Get() == Other.Get(); }

private:
    // Mutable: a read through a const reference writes the resolved address back.
    mutable std::uintptr_t Handle = 0;
};

static_assert(sizeof(FObjectPtr) == sizeof(UObject*));
static_assert(std::atomic_ref<std::uintptr_t>::required_alignment == alignof(std::uintptr_t));

// TObjectPtr: a typed FObjectPtr for reflected members, used as a T* (UE's TObjectPtr). T derives from UObject as its
// first base, so the addresses convert without the complete type.
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
