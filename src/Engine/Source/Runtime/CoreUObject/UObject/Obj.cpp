#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Stack.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

IMPLEMENT_CLASS(UObject, "Object")

FString UObject::GetPathName(const UObject* StopOuter) const {
    FString Result;
    if (this == StopOuter)
        return Result;
    UObject* ObjOuter = GetOuter();
    if (ObjOuter != nullptr && ObjOuter != StopOuter) {
        Result = ObjOuter->GetPathName(StopOuter);
        bool bOuterIsPackage = ObjOuter->IsA<UPackage>();
        bool bOuterOuterIsPackage = ObjOuter->GetOuter() != nullptr && ObjOuter->GetOuter()->IsA<UPackage>();
        Result += !bOuterIsPackage && bOuterOuterIsPackage ? u":" : u".";
    }
    Result += GetName();
    return Result;
}

FString UObject::GetFullName() const {
    FString Result = GetClass() ? GetClass()->GetName() : FString("None");
    Result += u" ";
    Result += GetPathName();
    return Result;
}

UPackage* UObject::GetOutermost() const {
    const UObject* Top = this;
    while (Top->GetOuter() != nullptr)
        Top = Top->GetOuter();
    return static_cast<UPackage*>(const_cast<UObject*>(Top));
}

bool IsObjectOfCastFlags(const UObject* Object, uint64 CastFlags) {
    UClass* Class = Object->GetClass();
    return Class != nullptr && Class->HasAnyCastFlag(CastFlags);
}

bool UObject::IsA(const UClass* SomeBase) const {
    UClass* Class = GetClass();
    return Class != nullptr && Class->IsChildOf(SomeBase);
}

UObject* UObject::GetTypedOuter(const UClass* Target) const {
    for (UObject* Outer = GetOuter(); Outer != nullptr; Outer = Outer->GetOuter())
        if (Outer->IsA(Target))
            return Outer;
    return nullptr;
}

bool UObject::IsInOuter(const UObject* SomeOuter) const {
    for (UObject* Outer = GetOuter(); Outer != nullptr; Outer = Outer->GetOuter())
        if (Outer == SomeOuter)
            return true;
    return false;
}

bool UObject::IsTemplate(EObjectFlags TemplateTypes) const {
    for (const UObject* Object = this; Object != nullptr; Object = Object->GetOuter())
        if (Object->HasAnyFlags(TemplateTypes))
            return true;
    return false;
}

UObject* UObject::GetArchetype() const {
    if (FLinkerLoad* ObjectLinker = GetLinker())
        if (UObject* Archetype = ObjectLinker->GetExportTemplate(GetLinkerIndex()))
            return Archetype;
    return GetArchetypeFromRequiredInfo(GetClass(), GetOuter(), GetFName(), GetFlags());
}

namespace {

//! UE loads every export of an imported package, so it finds archetypes that this loader has not created yet.
UObject* FindObjectWithOuter(UObject* Outer, const UClass* Class, FName Name) {
    if (UObject* Found = StaticFindObjectFast(Class, Outer, Name))
        return Found;
    if (FLinkerLoad* OuterLinker = Outer->GetLinker())
        OuterLinker->CreateSubobjectExport(Outer->GetLinkerIndex(), Name);
    return StaticFindObjectFast(Class, Outer, Name);
}

}

UObject* UObject::GetArchetypeFromRequiredInfo(UClass* Class, const UObject* Outer, FName Name, EObjectFlags ObjectFlags) {
    if (ObjectFlags & RF_ClassDefaultObject) {
        UClass* Super = Class->GetSuperClass();
        return Super ? Super->GetDefaultObject() : nullptr;
    }
    if (Outer == nullptr || Outer->IsA<UPackage>())
        return Class->GetDefaultObject();
    UObject* ArchetypeToSearch = Outer->GetArchetype();
    if (ArchetypeToSearch == nullptr)
        return Class->GetDefaultObject();
    if (UObject* Found = FindObjectWithOuter(ArchetypeToSearch, Class, Name))
        return Found;
    //! UE 4.7 adds the archetypes of inherited components and of the components of blueprint classes.
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_7)
        return Class->GetDefaultObject();
    UObject* Result = nullptr;
    if ((ObjectFlags & RF_InheritableComponentTemplate) && Outer->IsA<UClass>()) {
        for (const UClass* Super = static_cast<const UClass*>(Outer)->GetSuperClass(); Super != nullptr && Super->HasAllClassFlags(CLASS_CompiledFromBlueprint);
             Super = Super->GetSuperClass()) {
            Result = FindObjectWithOuter(const_cast<UClass*>(Super), Class, Name);
            if (Result != nullptr)
                break;
        }
    } else
        Result = ArchetypeToSearch->GetClass()->FindArchetype(Class, Name);
    return Result != nullptr ? Result : Class->GetDefaultObject();
}

UWorld* UObject::GetWorld() const {
    if (UObject* Outer = GetOuter())
        return Outer->GetWorld();
    return nullptr;
}

UFunction* UObject::FindFunction(FName InName) const {
    return GetClass()->FindFunctionByName(InName);
}

UFunction* UObject::FindFunctionChecked(FName InName) const {
    UFunction* Function = FindFunction(InName);
    if (Function == nullptr) {
        FString Message = FString("Failed to find function ") + NameToString(InName) + FString(" in ") + GetFullName();
        if (FFrame* Frame = FFrame::GetThreadLocalTopStackFrame())
            Frame->KismetExecutionMessage("MissingFunction", Message.ToUtf8(), EBlueprintExceptionType::NonFatalError);
        else
            ReportLoadProblem("script.function_missing", Message);
    }
    return Function;
}

void UObject::ConditionalPostLoad() {
    if (!HasAnyFlags(RF_NeedPostLoad))
        return;
    ClearFlags(RF_NeedPostLoad);
    PostLoad();
    SetFlags(RF_LoadCompleted);
}

void UObject::SerializeScriptProperties(FArchive& Ar) {
    GetClass()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
}

void UObject::Serialize(FArchive& Ar) {
    SerializeScriptProperties(Ar);
    if (Ar.IsError())
        return;
    bool bHasGuid = false;
    Ar << bHasGuid;
    if (bHasGuid) {
        FGuid Guid;
        Ar << Guid;
    }
}

FString GetNameSafe(const UObject* Object) {
    return Object ? Object->GetName() : FString("None");
}

FString GetPathNameSafe(const UObject* Object) {
    return Object ? Object->GetPathName() : FString("None");
}

}
