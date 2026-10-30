#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"

// The class checks of TSoftObjectPtr and TSoftClassPtr, which need UClass.

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
