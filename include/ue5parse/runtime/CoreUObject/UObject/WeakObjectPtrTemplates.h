#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

// TWeakObjectPtr: the typed FWeakObjectPtr, same memory (a reflected FWeakObjectProperty).
template <class T>
class TWeakObjectPtr {
public:
    TWeakObjectPtr() = default;
    TWeakObjectPtr(const T* Object) : Weak(reinterpret_cast<const UObject*>(Object)) {}
    TWeakObjectPtr& operator=(const T* Object) {
        Weak = reinterpret_cast<const UObject*>(Object);
        return *this;
    }

    // As UE, T may be incomplete here; every UObject class derives from UObject alone, so the address is the same.
    T* Get() const { return reinterpret_cast<T*>(Weak.Get()); }
    T* operator->() const { return Get(); }
    T& operator*() const { return *Get(); }
    explicit operator bool() const { return Get() != nullptr; }
    bool IsValid() const { return Get() != nullptr; }
    bool IsStale() const { return Weak.IsStale(); }
    void Reset() { Weak.Reset(); }
    bool operator==(const TWeakObjectPtr& Other) const { return Weak == Other.Weak; }
    bool operator==(const T* Other) const { return Get() == Other; }
    uint32 GetTypeHash() const { return Weak.GetTypeHash(); }

private:
    FWeakObjectPtr Weak;
};

template <class T>
uint32 GetTypeHash(const TWeakObjectPtr<T>& Pointer) {
    return Pointer.GetTypeHash();
}

}
