#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

namespace ue5parse::runtime {

template <class T>
UObject* TSoftObjectPtr<T>::CastToT(UObject* Object) {
    return Object != nullptr && Object->IsA(T::StaticClass()) ? Object : nullptr;
}

template <class T>
UClass* TSoftClassPtr<T>::CastToClass(UObject* Object) {
    UClass* Class = Cast<UClass>(Object);
    return Class != nullptr && Class->IsChildOf(T::StaticClass()) ? Class : nullptr;
}

}
