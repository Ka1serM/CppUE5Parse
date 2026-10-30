#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/UObjectArray.h"

namespace ue5parse::runtime {

// FWeakObjectPtr: an object index and the slot's serial number. Resolves to null once the object is destroyed or
// marked as garbage.
struct FWeakObjectPtr {
    FWeakObjectPtr() = default;
    FWeakObjectPtr(const UObject* Object) { *this = Object; }
    FWeakObjectPtr& operator=(const UObject* Object);

    UObject* Get() const;
    // The object even when it is garbage (not yet destroyed), as Get(true) in UE.
    UObject* GetEvenIfUnreachable() const;
    bool IsValid() const { return Get() != nullptr; }
    // Whether it once pointed at an object that is gone now.
    bool IsStale() const;
    void Reset() {
        ObjectIndex = 0;
        ObjectSerialNumber = 0;
    }
    bool operator==(const FWeakObjectPtr& Other) const {
        return ObjectIndex == Other.ObjectIndex && ObjectSerialNumber == Other.ObjectSerialNumber;
    }
    bool HasSameIndexAndSerialNumber(const FWeakObjectPtr& Other) const { return *this == Other; }
    uint32 GetTypeHash() const { return uint32(ObjectIndex) ^ uint32(ObjectSerialNumber); }

    // Serial number 0 is the null pointer (UE's InvalidWeakObjectIndex), so zeroed memory is null.
    int32 ObjectIndex = 0;
    int32 ObjectSerialNumber = 0;
};

}
