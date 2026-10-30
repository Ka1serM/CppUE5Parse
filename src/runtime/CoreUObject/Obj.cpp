#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

IMPLEMENT_CLASS(UObject, "Object")

FString UObject::GetPathName(const UObject* StopOuter) const {
    FString Result;
    if (this == StopOuter) return Result;
    UObject* ObjOuter = GetOuter();
    if (ObjOuter != nullptr && ObjOuter != StopOuter) {
        Result = ObjOuter->GetPathName(StopOuter);
        // ':' after a top-level object of a package, '.' elsewhere (UE's SUBOBJECT_DELIMITER).
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
    while (Top->GetOuter() != nullptr) Top = Top->GetOuter();
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
        if (Outer->IsA(Target)) return Outer;
    return nullptr;
}

bool UObject::IsInOuter(const UObject* SomeOuter) const {
    for (UObject* Outer = GetOuter(); Outer != nullptr; Outer = Outer->GetOuter())
        if (Outer == SomeOuter) return true;
    return false;
}

bool UObject::IsTemplate(EObjectFlags TemplateTypes) const {
    for (const UObject* Object = this; Object != nullptr; Object = Object->GetOuter())
        if (Object->HasAnyFlags(TemplateTypes)) return true;
    return false;
}

UObject* UObject::GetArchetype() const {
    // A loaded object's template is in its export map entry.
    if (FLinkerLoad* ObjectLinker = GetLinker())
        if (UObject* Archetype = ObjectLinker->GetExportTemplate(GetLinkerIndex())) return Archetype;
    // A subobject of a template: the archetype's subobject of the same name (UE's GetArchetypeFromRequiredInfo).
    UObject* Outer = GetOuter();
    if (Outer != nullptr && !HasAnyFlags(RF_ClassDefaultObject) && !Outer->IsA<UPackage>()) {
        UObject* OuterArchetype = Outer->GetArchetype();
        if (OuterArchetype != nullptr) {
            if (UObject* Found = StaticFindObjectFast(nullptr, OuterArchetype, GetFName()))
                if (Found->GetClass() == GetClass() || Found->IsA(GetClass())) return Found;
        }
    }
    UClass* Class = GetClass();
    if (HasAnyFlags(RF_ClassDefaultObject)) {
        UClass* Super = Class->GetSuperClass();
        return Super ? Super->GetDefaultObject() : nullptr;
    }
    return Class->GetDefaultObject();
}

UWorld* UObject::GetWorld() const {
    if (UObject* Outer = GetOuter()) return Outer->GetWorld();
    return nullptr;
}

UFunction* UObject::FindFunction(FName InName) const { return GetClass()->FindFunctionByName(InName); }

UFunction* UObject::FindFunctionChecked(FName InName) const {
    UFunction* Function = FindFunction(InName);
    if (Function == nullptr) {
        FString Message = FString("Failed to find function ") + NameToString(InName) + FString(" in ") + GetFullName();
        if (FFrame* Frame = FFrame::GetThreadLocalTopStackFrame()) {
            Frame->KismetExecutionMessage("MissingFunction", Message.ToUtf8(), EBlueprintExceptionType::FatalError);
        } else {
            ReportLoadProblem("script.function_missing", Message);
        }
    }
    return Function;
}

void UObject::SerializeScriptProperties(FArchive& Ar) { GetClass()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this)); }

void UObject::Serialize(FArchive& Ar) {
    SerializeScriptProperties(Ar);
    if (Ar.IsError()) return;
    // The lazy object guid (FLazyObjectPtr::PossiblySerializeObjectGuid).
    bool bHasGuid = false;
    Ar << bHasGuid;
    if (bHasGuid) {
        FGuid Guid;
        Ar << Guid;
    }
}

FString GetNameSafe(const UObject* Object) { return Object ? Object->GetName() : FString("None"); }

FString GetPathNameSafe(const UObject* Object) { return Object ? Object->GetPathName() : FString("None"); }

}
