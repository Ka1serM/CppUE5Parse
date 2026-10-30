#include "ue5parse/runtime/CoreUObject/UObject/UObjectArray.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/FieldPath.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"

namespace ue5parse::runtime {

FUObjectArray GUObjectArray;

int32 FUObjectArray::AllocateIndex(UObjectBase* Object) {
    std::lock_guard Lock(Mutex);
    int32 Index;
    if (FreeIndices.Num() > 0) {
        Index = FreeIndices.Pop();
    } else {
        Index = NumItems++;
        int32 Chunk = Index / ChunkSize;
        UE5PARSE_CHECK(Chunk < MaxChunks);
        if (Chunks[Chunk] == nullptr) Chunks[Chunk] = new FUObjectItem[ChunkSize];
    }
    FUObjectItem* Item = IndexToObject(Index);
    Item->Object = Object;
    Item->SerialNumber = 0;
    return Index;
}

void FUObjectArray::FreeIndex(int32 Index) {
    std::lock_guard Lock(Mutex);
    FUObjectItem* Item = IndexToObject(Index);
    Item->Object = nullptr;
    Item->SerialNumber = 0;
    FreeIndices.Add(Index);
}

FUObjectItem* FUObjectArray::IndexToObject(int32 Index) {
    if (Index < 0 || Index >= NumItems) return nullptr;
    return &Chunks[Index / ChunkSize][Index % ChunkSize];
}

int32 FUObjectArray::AllocateSerialNumber(int32 Index) {
    std::lock_guard Lock(Mutex);
    FUObjectItem* Item = IndexToObject(Index);
    if (Item->SerialNumber == 0) Item->SerialNumber = NextSerialNumber++;
    return Item->SerialNumber;
}

FWeakObjectPtr& FWeakObjectPtr::operator=(const UObject* Object) {
    if (Object == nullptr) {
        Reset();
    } else {
        ObjectIndex = Object->GetInternalIndex();
        ObjectSerialNumber = GUObjectArray.AllocateSerialNumber(ObjectIndex);
    }
    return *this;
}

UObject* FWeakObjectPtr::GetEvenIfUnreachable() const {
    if (ObjectSerialNumber == 0) return nullptr;
    FUObjectItem* Item = GUObjectArray.IndexToObject(ObjectIndex);
    if (Item == nullptr || Item->SerialNumber != ObjectSerialNumber) return nullptr;
    return static_cast<UObject*>(Item->Object);
}

UObject* FWeakObjectPtr::Get() const {
    UObject* Object = GetEvenIfUnreachable();
    return Object && !Object->IsGarbage() ? Object : nullptr;
}

bool FWeakObjectPtr::IsStale() const { return ObjectSerialNumber != 0 && Get() == nullptr; }

FFieldPath& FFieldPath::operator=(FField* Field) {
    Path.Empty();
    ResolvedField = Field;
    ResolvedOwner.Reset();
    if (Field == nullptr) return *this;
    // Innermost first: the field, its outer fields, then the owner object.
    FField* Current = Field;
    for (;;) {
        Path.Add(Current->GetFName());
        FFieldVariant Owner = Current->GetOwnerVariant();
        if (Owner.IsUObject()) {
            ResolvedOwner = Owner.ToUObject();
            break;
        }
        Current = Owner.ToField();
        if (Current == nullptr) break;
    }
    return *this;
}

FField* FFieldPath::Get() const {
    if (ResolvedField != nullptr) return ResolvedField;
    UStruct* Owner = Cast<UStruct>(ResolvedOwner.Get());
    if (Owner == nullptr || Path.Num() == 0) return nullptr;
    // Outermost field name last: find it on the owner, then walk inward through inner properties.
    FField* Field = Owner->FindPropertyByName(Path.Last());
    for (int32 i = Path.Num() - 2; Field != nullptr && i >= 0; i--) {
        FField* Inner = nullptr;
        if (auto* Array = CastField<FArrayProperty>(Field)) {
            Inner = Array->Inner;
        } else if (auto* Set = CastField<FSetProperty>(Field)) {
            Inner = Set->ElementProp;
        } else if (auto* Map = CastField<FMapProperty>(Field)) {
            Inner = Map->KeyProp && Map->KeyProp->GetFName() == Path[i] ? Map->KeyProp : Map->ValueProp;
        } else if (auto* Enum = CastField<FEnumProperty>(Field)) {
            Inner = Enum->UnderlyingProp;
        }
        Field = Inner && Inner->GetFName() == Path[i] ? Inner : nullptr;
    }
    ResolvedField = Field;
    return Field;
}

FString FFieldPath::ToString() const {
    UObject* Owner = ResolvedOwner.Get();
    if (Owner == nullptr || Path.Num() == 0) return FString();
    FString Result = Owner->GetPathName();
    for (int32 i = Path.Num() - 1; i >= 0; i--) {
        Result += u":";
        Result += NameToString(Path[i]);
    }
    return Result;
}

void FFieldPath::Generate(const FString& PathString) {
    Reset();
    // "<owner path>:<field>[:<inner>...]": the owner path itself can contain one ':' (a subobject), so the fields are
    // what follows the owner that resolves.
    TArray<FString> Parts;
    PathString.ParseIntoArray(Parts, FString(":"), true);
    for (int32 Split = Parts.Num() - 1; Split >= 1; Split--) {
        FString OwnerPath = Parts[0];
        for (int32 i = 1; i < Split; i++) OwnerPath += FString(":") + Parts[i];
        UStruct* Owner = Cast<UStruct>(StaticFindObject(nullptr, nullptr, OwnerPath));
        if (Owner == nullptr) continue;
        ResolvedOwner = Owner;
        for (int32 i = Parts.Num() - 1; i >= Split; i--) Path.Add(MakeName(Parts[i]));
        return;
    }
}

}
