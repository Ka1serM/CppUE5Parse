#pragma once

#include "ue5parse/runtime/Core/UObject/NameTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/ObjectMacros.h"

namespace ue5parse::runtime {

class FField;
class UObject;
class UStruct;
class UClass;

// The class of an FField: its name, its cast flags (its own and its supers'), and its super. One static instance per
// FField class, as UE's FFieldClass.
class FFieldClass {
public:
    FFieldClass(const char* Name, uint64 Id, uint64 CastFlags, FFieldClass* SuperClass)
        : Name(Name), Id(Id), CastFlags(CastFlags | (SuperClass ? SuperClass->CastFlags : 0)), SuperClass(SuperClass) {}
    FFieldClass(const FFieldClass&) = delete;
    FFieldClass& operator=(const FFieldClass&) = delete;

    const char* GetName() const { return Name; }
    uint64 GetId() const { return Id; }
    uint64 GetCastFlags() const { return CastFlags; }
    bool HasAnyCastFlags(uint64 Flags) const { return (CastFlags & Flags) != 0; }
    bool HasAllCastFlags(uint64 Flags) const { return (CastFlags & Flags) == Flags; }
    FFieldClass* GetSuperClass() const { return SuperClass; }
    bool IsChildOf(const FFieldClass* Other) const { return (CastFlags & Other->Id) != 0 || this == Other; }

private:
    const char* Name;
    uint64 Id;
    uint64 CastFlags;
    FFieldClass* SuperClass;
};

// The owner of a field: a UStruct (or another UObject), or an outer FField (an array's inner property).
class FFieldVariant {
public:
    FFieldVariant() = default;
    FFieldVariant(UObject* Object) : Object(Object) {}
    FFieldVariant(FField* Field) : Field(Field) {}

    bool IsUObject() const { return Object != nullptr; }
    bool IsValid() const { return Object != nullptr || Field != nullptr; }
    UObject* ToUObject() const { return Object; }
    FField* ToField() const { return Field; }

private:
    UObject* Object = nullptr;
    FField* Field = nullptr;
};

#define DECLARE_FIELD(TClass, TSuperClass, TStaticFlags)                                                               \
public:                                                                                                                \
    using Super = TSuperClass;                                                                                         \
    static constexpr uint64 StaticClassCastFlags = TStaticFlags;                                                       \
    static FFieldClass* StaticClass();

#define IMPLEMENT_FIELD(TClass)                                                                                        \
    FFieldClass* TClass::StaticClass() {                                                                               \
        static FFieldClass Class(#TClass, TClass::StaticClassCastFlags, TClass::StaticClassCastFlags,                  \
                                 TClass::Super::StaticClass());                                                        \
        return &Class;                                                                                                 \
    }

// FField: a property of a struct, class or function. Not a UObject, as in UE 4.25 and later.
class FField {
public:
    using Super = FField;
    static constexpr uint64 StaticClassCastFlags = 0;
    static FFieldClass* StaticClass();

    FField(FFieldClass* InClass, FFieldVariant InOwner, FName InName, EObjectFlags InFlags)
        : ClassPrivate(InClass), Owner(InOwner), NamePrivate(InName), FlagsPrivate(InFlags) {}
    virtual ~FField() = default;
    FField(const FField&) = delete;
    FField& operator=(const FField&) = delete;

    FFieldClass* GetClass() const { return ClassPrivate; }
    FName GetFName() const { return NamePrivate; }
    FString GetName() const { return NameToString(NamePrivate); }
    EObjectFlags GetFlags() const { return FlagsPrivate; }
    bool HasAnyFlags(EObjectFlags Flags) const { return (FlagsPrivate & Flags) != 0; }
    bool HasAllFlags(EObjectFlags Flags) const { return (FlagsPrivate & Flags) == Flags; }
    bool IsA(const FFieldClass* FieldClass) const { return ClassPrivate->IsChildOf(FieldClass); }
    template <class T>
    bool IsA() const {
        return IsA(T::StaticClass());
    }

    FFieldVariant GetOwnerVariant() const { return Owner; }
    // The UObject that owns this field, through outer fields.
    UObject* GetOwnerUObject() const;
    // The struct that owns this field, through outer fields; null when it is no UStruct.
    UStruct* GetOwnerStruct() const;
    UClass* GetOwnerClass() const;
    // "Owner:Name", for messages.
    FString GetPathName() const;

    FFieldClass* ClassPrivate;
    FFieldVariant Owner;
    FField* Next = nullptr;
    FName NamePrivate;
    EObjectFlags FlagsPrivate;
};

template <class T>
T* CastField(FField* Src) {
    return Src && Src->GetClass()->HasAnyCastFlags(T::StaticClassCastFlags) ? static_cast<T*>(Src) : nullptr;
}
template <class T>
const T* CastField(const FField* Src) {
    return Src && Src->GetClass()->HasAnyCastFlags(T::StaticClassCastFlags) ? static_cast<const T*>(Src) : nullptr;
}
// The exact class, not a subclass.
template <class T>
T* ExactCastField(FField* Src) {
    return Src && Src->GetClass() == T::StaticClass() ? static_cast<T*>(Src) : nullptr;
}
template <class T>
T* CastFieldChecked(FField* Src) {
    T* Result = CastField<T>(Src);
    UE5PARSE_CHECK(Result != nullptr);
    return Result;
}

}
