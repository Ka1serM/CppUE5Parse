#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

void FScriptDelegate::BindUFunction(UObject* InObject, FName InFunctionName) {
    Object = InObject;
    FunctionName = InFunctionName;
}

bool FScriptDelegate::IsBound() const {
    if (FunctionName.isNone()) return false;
    UObject* ObjectPtr = Object.Get();
    return ObjectPtr != nullptr && ObjectPtr->FindFunction(FunctionName) != nullptr;
}

bool FScriptDelegate::IsBoundToObject(const UObject* InObject) const {
    return InObject != nullptr && Object.Get() == InObject;
}

void FScriptDelegate::ProcessDelegate(void* Parameters) const {
    UObject* ObjectPtr = Object.Get();
    if (ObjectPtr == nullptr || FunctionName.isNone()) return;
    UFunction* Function = ObjectPtr->FindFunctionChecked(FunctionName);
    if (Function != nullptr) ObjectPtr->ProcessEvent(Function, Parameters);
}

namespace {

bool IsCompactable(const FScriptDelegate& Delegate) {
    return Delegate.GetFunctionName().isNone() || Delegate.GetUObjectEvenIfUnreachable() == nullptr;
}

void CompactInvocationList(TArray<FScriptDelegate>& List) { List.RemoveAllSwap(IsCompactable); }

}

bool FMulticastScriptDelegate::IsBound() const {
    for (const FScriptDelegate& Delegate : InvocationList)
        if (Delegate.IsBound()) return true;
    return false;
}

void FMulticastScriptDelegate::AddUnique(const FScriptDelegate& Delegate) {
    InvocationList.AddUnique(Delegate);
    CompactInvocationList(InvocationList);
}

void FMulticastScriptDelegate::Add(const FScriptDelegate& Delegate) {
    CompactInvocationList(InvocationList);
    InvocationList.Add(Delegate);
}

void FMulticastScriptDelegate::Remove(const FScriptDelegate& Delegate) {
    InvocationList.RemoveSingleSwap(Delegate);
    CompactInvocationList(InvocationList);
}

void FMulticastScriptDelegate::Remove(const UObject* InObject, FName InFunctionName) {
    for (int32 i = 0; i < InvocationList.Num(); i++) {
        const FScriptDelegate& Delegate = InvocationList[i];
        if (Delegate.GetFunctionName() == InFunctionName && Delegate.GetUObjectEvenIfUnreachable() == InObject) {
            InvocationList.RemoveAtSwap(i);
            break;
        }
    }
    CompactInvocationList(InvocationList);
}

void FMulticastScriptDelegate::RemoveAll(const UObject* Object) {
    for (int32 i = InvocationList.Num() - 1; i >= 0; i--) {
        const FScriptDelegate& Binding = InvocationList[i];
        if (Binding.IsBoundToObject(Object) || IsCompactable(Binding)) InvocationList.RemoveAtSwap(i);
    }
}

void FMulticastScriptDelegate::ProcessMulticastDelegate(void* Parameters) const {
    if (InvocationList.Num() == 0) return;
    // A callback can change the list: call a copy.
    TArray<FScriptDelegate> Copy(InvocationList);
    for (const FScriptDelegate& Delegate : Copy) {
        if (Delegate.IsBound()) {
            Delegate.ProcessDelegate(Parameters);
        } else if (IsCompactable(Delegate)) {
            InvocationList.RemoveSingleSwap(Delegate);
        }
    }
}

TArray<UObject*> FMulticastScriptDelegate::GetAllObjects() const {
    TArray<UObject*> Objects;
    for (const FScriptDelegate& Delegate : InvocationList)
        if (UObject* Object = Delegate.GetUObject()) Objects.Add(Object);
    return Objects;
}

FSoftObjectPath::FSoftObjectPath(const UObject* Object) {
    if (Object != nullptr) SetPath(Object->GetPathName());
}

void FSoftObjectPath::SetPath(const FString& Path) {
    Reset();
    // "/Package/Path.Asset:Sub.Path" (a path without '.' names a package: the asset name stays None).
    if (Path.IsEmpty() || Path.Equals(FString("None"), ESearchCase::IgnoreCase)) return;
    FString Clean = Path;
    // An export text path: Class'/Path.Asset'.
    int32 Quote = Clean.Find(FString("'"));
    if (Quote != INDEX_NONE && Clean.EndsWith(FString("'"))) Clean = Clean.Mid(Quote + 1, Clean.Len() - Quote - 2);
    if (Clean.Len() == 0 || Clean[0] != u'/') return;
    int32 Dot = Clean.Find(FString("."), ESearchCase::CaseSensitive);
    int32 Colon = Clean.Find(FString(":"), ESearchCase::CaseSensitive);
    if (Dot == INDEX_NONE) {
        AssetPath.PackageName = MakeName(Clean);
        return;
    }
    AssetPath.PackageName = MakeName(Clean.Left(Dot));
    if (Colon != INDEX_NONE && Colon > Dot) {
        AssetPath.AssetName = MakeName(Clean.Mid(Dot + 1, Colon - Dot - 1));
        SubPathString = Clean.Mid(Colon + 1);
    } else {
        AssetPath.AssetName = MakeName(Clean.Mid(Dot + 1));
    }
}

FString FSoftObjectPath::ToString() const {
    if (IsNull()) return FString();
    FString Result = NameToString(AssetPath.PackageName);
    if (!AssetPath.AssetName.isNone()) {
        Result += u".";
        Result += NameToString(AssetPath.AssetName);
    }
    if (!SubPathString.IsEmpty()) {
        Result += u":";
        Result += SubPathString;
    }
    return Result;
}

UObject* FSoftObjectPath::ResolveObject() const {
    if (IsNull()) return nullptr;
    return StaticFindObject(nullptr, nullptr, ToString());
}

UObject* FSoftObjectPath::TryLoad() const {
    if (IsNull()) return nullptr;
    return StaticLoadObject(nullptr, nullptr, ToString());
}

uint32 GetTypeHash(const FSoftObjectPath& Path) {
    uint32 Hash = static_cast<uint32>(FNameHasher()(Path.AssetPath.PackageName));
    Hash = Hash * 31 + static_cast<uint32>(FNameHasher()(Path.AssetPath.AssetName));
    return Hash * 31 + GetTypeHash(Path.SubPathString);
}

UObject* FSoftObjectPtr::Get() const {
    UObject* Object = WeakPtr.Get();
    if (Object == nullptr && !ObjectID.IsNull()) {
        Object = ObjectID.ResolveObject();
        WeakPtr = Object;
    }
    return Object;
}

UObject* FSoftObjectPtr::LoadSynchronous() const {
    UObject* Object = Get();
    if (Object == nullptr && !ObjectID.IsNull()) {
        Object = ObjectID.TryLoad();
        WeakPtr = Object;
    }
    return Object;
}

}
