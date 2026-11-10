#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectArray.h"

namespace ue5parse::runtime {

struct FWeakObjectPtr {
    FWeakObjectPtr() = default;
    FWeakObjectPtr(const UObject* Object) { *this = Object; }
    FWeakObjectPtr& operator=(const UObject* Object);

    UObject* Get() const;
    UObject* GetEvenIfUnreachable() const;
    bool IsValid() const { return Get() != nullptr; }
    bool IsStale() const;
    void Reset() {
        ObjectIndex = 0;
        ObjectSerialNumber = 0;
    }
    bool operator==(const FWeakObjectPtr& Other) const { return ObjectIndex == Other.ObjectIndex && ObjectSerialNumber == Other.ObjectSerialNumber; }
    uint32 GetTypeHash() const { return uint32(ObjectIndex) ^ uint32(ObjectSerialNumber); }

    int32 ObjectIndex = 0;
    int32 ObjectSerialNumber = 0;
};

}
