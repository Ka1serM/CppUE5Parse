#pragma once

#include <any>
#include <memory>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::Chaos {

//! Shares each object between the pointers that store its tag, as UE's FChaosArchiveContext does.
class FChaosArchive : public FArchive {
public:
    explicit FChaosArchive(FArchive& InInnerArchive);

    void Serialize(void* Data, int64 Length) override;
    int64 Tell() const override { return InnerArchive.Tell(); }
    int64 TotalSize() const override { return InnerArchive.TotalSize(); }
    void Seek(int64 InPos) override;
    std::span<const std::byte> ReadView(int64 Length) override;
    int64 GetCookedPosition() const override { return InnerArchive.GetCookedPosition(); }
    int64 GetExportChunkPosition() const override { return InnerArchive.GetExportChunkPosition(); }
    FLinkerLoad* GetLinker() override { return InnerArchive.GetLinker(); }
    FUnknownProperties* GetUnknownProperties() override { return InnerArchive.GetUnknownProperties(); }

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override;
    FArchive& operator<<(UObject*& Value) override;
    FArchive& operator<<(FObjectPtr& Value) override;
    FArchive& operator<<(FSoftObjectPath& Value) override;
    FArchive& operator<<(FSoftObjectPtr& Value) override;
    FArchive& operator<<(FWeakObjectPtr& Value) override;

    template <class T>
    void SerializePtr(std::shared_ptr<T>& Obj);
    void ReportErrorTo(FArchive& Ar) const;

private:
    bool SerializeTag(int32& Tag);
    void TakeInnerError();

    FArchive& InnerArchive;
    TArray<std::any> TagToObject;
};

template <class T>
void FChaosArchive::SerializePtr(std::shared_ptr<T>& Obj) {
    Obj.reset();
    int32 Tag = 0;
    if (!SerializeTag(Tag))
        return;
    std::any& Slot = TagToObject[Tag];
    if (Slot.has_value()) {
        const std::shared_ptr<T>* Shared = std::any_cast<std::shared_ptr<T>>(&Slot);
        if (Shared == nullptr) {
            SetError(ErrorCode::InvalidData, "FChaosArchive.SerializePtr");
            return;
        }
        Obj = *Shared;
        return;
    }
    Obj = std::make_shared<T>();
    Obj->Serialize(*this);
    TagToObject[Tag] = Obj;
}

template <class T>
FChaosArchive& operator<<(FChaosArchive& Ar, std::shared_ptr<T>& Obj) {
    Ar.SerializePtr(Obj);
    return Ar;
}

//! UE's FChaosArchive serializes every array element by element, so a bool element takes four bytes.
template <class T>
FChaosArchive& operator<<(FChaosArchive& Ar, TArray<T>& Array) {
    const int32 Count = Ar.ReadCount(1);
    Array.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << Array[Array.AddDefaulted()];
    if (Ar.IsError())
        Array.Empty();
    return Ar;
}

}
