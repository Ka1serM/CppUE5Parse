#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

template <class T>
class TWeakObjectPtr {
public:
    TWeakObjectPtr() = default;
    TWeakObjectPtr(const T* Object) : Weak(reinterpret_cast<const UObject*>(Object)) {}
    TWeakObjectPtr& operator=(const T* Object) {
        Weak = reinterpret_cast<const UObject*>(Object);
        return *this;
    }

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
