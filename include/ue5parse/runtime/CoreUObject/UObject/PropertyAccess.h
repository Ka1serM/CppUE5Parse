#pragma once

#include <type_traits>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"

// Reflection lookups by name: FindFProperty, TBaseStructure, and typed reads of property values for code that reads
// Blueprint and data asset properties without a C++ member (UE's FindFProperty and the property value accessors).

namespace ue5parse::runtime {

template <class T>
T* FindFProperty(const UStruct* Owner, FName FieldName) {
    return Owner ? CastField<T>(Owner->FindPropertyByName(FieldName)) : nullptr;
}

// The UScriptStruct of a native C++ struct: T::StaticStruct() for USTRUCTs (UnrealHeaderTool defines it),
// the core math structs by their names.
template <class T>
struct TBaseStructure {
    static UScriptStruct* Get() { return T::StaticStruct(); }
};

#define DECLARE_BASE_STRUCTURE(TType, TName)                                                                           \
    template <>                                                                                                        \
    struct TBaseStructure<TType> {                                                                                     \
        static UScriptStruct* Get() {                                                                                  \
            static UScriptStruct* const Struct = FindNativeStruct(FName(TName));                                       \
            return Struct;                                                                                             \
        }                                                                                                              \
    };

DECLARE_BASE_STRUCTURE(FVector, "Vector")
DECLARE_BASE_STRUCTURE(FVector2D, "Vector2D")
DECLARE_BASE_STRUCTURE(FVector4, "Vector4")
DECLARE_BASE_STRUCTURE(FVector3f, "Vector3f")
DECLARE_BASE_STRUCTURE(FRotator, "Rotator")
DECLARE_BASE_STRUCTURE(FQuat, "Quat")
DECLARE_BASE_STRUCTURE(FTransform, "Transform")
DECLARE_BASE_STRUCTURE(FLinearColor, "LinearColor")
DECLARE_BASE_STRUCTURE(FColor, "Color")
DECLARE_BASE_STRUCTURE(FGuid, "Guid")
DECLARE_BASE_STRUCTURE(FIntPoint, "IntPoint")
DECLARE_BASE_STRUCTURE(FIntVector, "IntVector")
DECLARE_BASE_STRUCTURE(FBox, "Box")
DECLARE_BASE_STRUCTURE(FSoftObjectPath, "SoftObjectPath")

template <class T>
struct TIsTArray : std::false_type {};
template <class T>
struct TIsTArray<TArray<T>> : std::true_type {
    using ElementType = T;
};

// Reads the value at ValuePtr, laid out as Property, into a C++ value. False when the property's type does not
// convert to T. Numbers convert between all integer and floating point types (enums give their number); bools read
// bitfields too; object pointers check the class; structs must be T's struct or a child of it; arrays convert each
// element.
template <class T>
bool GetPropertyValue(const FProperty* Property, const void* ValuePtr, T& Out) {
    if (Property == nullptr || ValuePtr == nullptr) return false;
    if constexpr (std::is_same_v<T, bool>) {
        const FBoolProperty* Bool = CastField<FBoolProperty>(Property);
        if (Bool == nullptr) return false;
        Out = Bool->GetPropertyValue(ValuePtr);
        return true;
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
        if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property)) Numeric = Enum->GetUnderlyingProperty();
        if (Numeric == nullptr) return false;
        if constexpr (std::is_floating_point_v<T>) {
            Out = Numeric->IsFloatingPoint() ? static_cast<T>(Numeric->GetFloatingPointPropertyValue(ValuePtr))
                                             : static_cast<T>(Numeric->GetSignedIntPropertyValue(ValuePtr));
        } else {
            Out = Numeric->IsFloatingPoint() ? static_cast<T>(Numeric->GetFloatingPointPropertyValue(ValuePtr))
                                             : static_cast<T>(Numeric->GetSignedIntPropertyValue(ValuePtr));
        }
        return true;
    } else if constexpr (std::is_same_v<T, FName>) {
        if (!Property->IsA<FNameProperty>()) return false;
        Out = *static_cast<const FName*>(ValuePtr);
        return true;
    } else if constexpr (std::is_same_v<T, FString>) {
        if (!Property->IsA<FStrProperty>()) return false;
        Out = *static_cast<const FString*>(ValuePtr);
        return true;
    } else if constexpr (std::is_same_v<T, FText>) {
        if (!Property->IsA<FTextProperty>()) return false;
        Out = *static_cast<const FText*>(ValuePtr);
        return true;
    } else if constexpr (std::is_same_v<T, FSoftObjectPtr>) {
        if (!Property->IsA<FSoftObjectProperty>()) return false;
        Out = *static_cast<const FSoftObjectPtr*>(ValuePtr);
        return true;
    } else if constexpr (std::is_same_v<T, FSoftObjectPath>) {
        if (Property->IsA<FSoftObjectProperty>()) {
            Out = static_cast<const FSoftObjectPtr*>(ValuePtr)->GetUniqueID();
            return true;
        }
        const FStructProperty* Struct = CastField<FStructProperty>(Property);
        if (Struct == nullptr || Struct->Struct != TBaseStructure<FSoftObjectPath>::Get()) return false;
        Out = *static_cast<const FSoftObjectPath*>(ValuePtr);
        return true;
    } else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<UObject, std::remove_cv_t<std::remove_pointer_t<T>>>) {
        const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property);
        if (Object == nullptr) {
            if (const FInterfaceProperty* Interface = CastField<FInterfaceProperty>(Property)) {
                Out = Cast<std::remove_cv_t<std::remove_pointer_t<T>>>(static_cast<const FScriptInterface*>(ValuePtr)->GetObject());
                return true;
            }
            return false;
        }
        UObject* Value = Object->GetObjectPropertyValue(ValuePtr);
        if (Value != nullptr && Cast<std::remove_cv_t<std::remove_pointer_t<T>>>(Value) == nullptr) return false;
        Out = static_cast<T>(Value);
        return true;
    } else if constexpr (TIsTArray<T>::value) {
        using TElement = typename TIsTArray<T>::ElementType;
        const FArrayProperty* Array = CastField<FArrayProperty>(Property);
        if (Array == nullptr) return false;
        FScriptArrayHelper Helper(Array, ValuePtr);
        Out.Empty(Helper.Num());
        for (int32 i = 0; i < Helper.Num(); i++) {
            TElement Element{};
            if (!GetPropertyValue(Array->Inner, Helper.GetRawPtr(i), Element)) return false;
            Out.Add(std::move(Element));
        }
        return true;
    } else {
        const FStructProperty* Struct = CastField<FStructProperty>(Property);
        UScriptStruct* Wanted = TBaseStructure<T>::Get();
        if (Struct == nullptr || Wanted == nullptr || !Struct->Struct->IsChildOf(Wanted)) return false;
        Wanted->CopyScriptStruct(&Out, ValuePtr);
        return true;
    }
}

// A property of a container (an object or a struct value laid out as Struct), by name.
template <class T>
bool GetPropertyValue_InContainer(const UStruct* Struct, const void* Container, FName Name, T& Out, int32 ArrayIndex = 0) {
    const FProperty* Property = Struct ? Struct->FindPropertyByName(Name) : nullptr;
    if (Property == nullptr || ArrayIndex < 0 || ArrayIndex >= Property->ArrayDim) return false;
    return GetPropertyValue(Property, Property->ContainerPtrToValuePtr<void>(Container, ArrayIndex), Out);
}

template <class T>
bool GetPropertyValue(const UObject* Object, FName Name, T& Out, int32 ArrayIndex = 0) {
    return Object != nullptr && GetPropertyValue_InContainer(Object->GetClass(), Object, Name, Out, ArrayIndex);
}

template <class T>
T GetPropertyValueOrDefault(const UObject* Object, FName Name, T Default = T{}) {
    T Value{};
    return GetPropertyValue(Object, Name, Value) ? Value : Default;
}

// Writes a C++ value into the value at ValuePtr, laid out as Property. False when T does not convert to the property's
// type: numbers convert to every numeric and enum property, object pointers must fit the property's class, structs
// must be the property's struct or a child of it.
template <class T>
bool SetPropertyValue(const FProperty* Property, void* ValuePtr, const T& Value) {
    if (Property == nullptr || ValuePtr == nullptr) return false;
    if constexpr (std::is_same_v<T, bool>) {
        const FBoolProperty* Bool = CastField<FBoolProperty>(Property);
        if (Bool == nullptr) return false;
        Bool->SetPropertyValue(ValuePtr, Value);
        return true;
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
        if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property)) Numeric = Enum->GetUnderlyingProperty();
        if (Numeric == nullptr) return false;
        if (Numeric->IsFloatingPoint()) Numeric->SetFloatingPointPropertyValue(ValuePtr, static_cast<double>(Value));
        else Numeric->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
        return true;
    } else if constexpr (std::is_same_v<T, FName>) {
        if (!Property->IsA<FNameProperty>()) return false;
        *static_cast<FName*>(ValuePtr) = Value;
        return true;
    } else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<UObject, std::remove_cv_t<std::remove_pointer_t<T>>>) {
        const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property);
        if (Object == nullptr) return false;
        Object->SetObjectPropertyValue(ValuePtr, const_cast<UObject*>(static_cast<const UObject*>(Value)));
        return true;
    } else {
        const FStructProperty* Struct = CastField<FStructProperty>(Property);
        UScriptStruct* Wanted = TBaseStructure<T>::Get();
        if (Struct == nullptr || Wanted == nullptr || !Wanted->IsChildOf(Struct->Struct)) return false;
        Struct->Struct->CopyScriptStruct(ValuePtr, &Value);
        return true;
    }
}

template <class T>
bool SetPropertyValue_InContainer(const UStruct* Struct, void* Container, FName Name, const T& Value, int32 ArrayIndex = 0) {
    const FProperty* Property = Struct ? Struct->FindPropertyByName(Name) : nullptr;
    if (Property == nullptr || ArrayIndex < 0 || ArrayIndex >= Property->ArrayDim) return false;
    return SetPropertyValue(Property, Property->ContainerPtrToValuePtr<void>(Container, ArrayIndex), Value);
}

template <class T>
bool SetPropertyValue(UObject* Object, FName Name, const T& Value, int32 ArrayIndex = 0) {
    return Object != nullptr && SetPropertyValue_InContainer(Object->GetClass(), Object, Name, Value, ArrayIndex);
}

// The address of a property's value in an object, with its property; null when the object's class has no such
// property.
inline void* GetPropertyValuePtr(UObject* Object, FName Name, FProperty** OutProperty = nullptr, int32 ArrayIndex = 0) {
    FProperty* Property = Object ? Object->GetClass()->FindPropertyByName(Name) : nullptr;
    if (OutProperty != nullptr) *OutProperty = Property;
    if (Property == nullptr || ArrayIndex < 0 || ArrayIndex >= Property->ArrayDim) return nullptr;
    return Property->ContainerPtrToValuePtr<void>(Object, ArrayIndex);
}

}
