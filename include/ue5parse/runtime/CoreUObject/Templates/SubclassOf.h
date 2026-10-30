#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

// TSubclassOf: a class that is T or a subclass of it, stored as a TObjectPtr<UClass> (a reflected FClassProperty). A class of
// another tree reads as null, as in UE.
template <class T>
class TSubclassOf {
public:
    TSubclassOf() = default;
    TSubclassOf(UClass* From) : Class(From) {}

    UClass* Get() const { return Class && Class->IsChildOf(T::StaticClass()) ? Class : nullptr; }
    operator UClass*() const { return Get(); }
    UClass* operator*() const { return Get(); }
    UClass* operator->() const { return Get(); }
    T* GetDefaultObject() const {
        UClass* Result = Get();
        return Result ? static_cast<T*>(Result->GetDefaultObject()) : nullptr;
    }
    bool operator==(const TSubclassOf& Other) const { return Class == Other.Class; }

private:
    TObjectPtr<UClass> Class;
};

}
