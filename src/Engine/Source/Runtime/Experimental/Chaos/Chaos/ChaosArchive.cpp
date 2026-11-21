#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

FChaosArchive::FChaosArchive(FArchive& InInnerArchive) : InnerArchive(InInnerArchive) {
    CopyStateFrom(InInnerArchive);
}

void FChaosArchive::Serialize(void* Data, int64 Length) {
    InnerArchive.Serialize(Data, Length);
    TakeInnerError();
}

void FChaosArchive::Seek(int64 InPos) {
    InnerArchive.Seek(InPos);
    TakeInnerError();
}

std::span<const std::byte> FChaosArchive::ReadView(int64 Length) {
    const std::span<const std::byte> View = InnerArchive.ReadView(Length);
    TakeInnerError();
    return View;
}

FArchive& FChaosArchive::operator<<(FName& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

FArchive& FChaosArchive::operator<<(UObject*& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

FArchive& FChaosArchive::operator<<(FObjectPtr& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

FArchive& FChaosArchive::operator<<(FSoftObjectPath& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

FArchive& FChaosArchive::operator<<(FSoftObjectPtr& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

FArchive& FChaosArchive::operator<<(FWeakObjectPtr& Value) {
    InnerArchive << Value;
    TakeInnerError();
    return *this;
}

bool FChaosArchive::SerializeTag(int32& Tag) {
    if (!CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::ChaosArchiveAdded, EGame::GAME_UE4_23)) {
        SetError(ErrorCode::UnsupportedVersion, "FChaosArchive.SerializeLegacy");
        return false;
    }
    bool bExists = false;
    *this << bExists;
    if (!bExists || IsError())
        return false;
    *this << Tag;
    if (IsError())
        return false;
    if (Tag < 0 || Tag > GetRemaining()) {
        SetError(ErrorCode::InvalidData, "FChaosArchive.SerializeTag");
        return false;
    }
    if (Tag >= TagToObject.Num())
        TagToObject.SetNum(Tag + 1);
    return true;
}

void FChaosArchive::ReportErrorTo(FArchive& Ar) const {
    if (IsError())
        Ar.SetError(GetError().code, GetError().where);
}

void FChaosArchive::TakeInnerError() {
    if (InnerArchive.IsError())
        SetError(InnerArchive.GetError().code, InnerArchive.GetError().where);
}

}
