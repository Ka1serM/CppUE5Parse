#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include <algorithm>

namespace ue5parse::runtime {

void SerializeStructInstance(FArchive& Ar, const UScriptStruct* Struct, void* Memory, int32 SerialSize) {
    if (SerialSize < 0 || SerialSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::InvalidData, "StructInstance.SerialSize");
        return;
    }
    const int64 End = Ar.Tell() + SerialSize;
    if (Struct == nullptr) {
        Ar.Seek(End);
        return;
    }
    Struct->SerializeItem(Ar, Memory);
    if (!Ar.IsError() && Ar.Tell() != End)
        Ar.SetError(ErrorCode::InvalidData, "StructInstance.SerialSize");
}

FInstancedStruct& FInstancedStruct::operator=(const FInstancedStruct& Other) {
    if (this == &Other)
        return *this;
    InitializeAs(Other.ScriptStruct);
    if (ScriptStruct != nullptr && Other.StructMemory != nullptr)
        ScriptStruct->CopyScriptStruct(StructMemory, Other.StructMemory);
    return *this;
}

void FInstancedStruct::InitializeAs(const UScriptStruct* InScriptStruct) {
    Reset();
    if (InScriptStruct == nullptr)
        return;
    ScriptStruct = InScriptStruct;
    const int32 Size = std::max(ScriptStruct->GetStructureSize(), 1);
    StructMemory = static_cast<uint8*>(RuntimeMalloc(static_cast<std::size_t>(Size), static_cast<std::size_t>(std::max(ScriptStruct->GetMinAlignment(), 16))));
    ScriptStruct->InitializeStruct(StructMemory);
}

bool FInstancedStruct::IsChildOf(const UScriptStruct* Base) const {
    return IsValid() && ScriptStruct->IsChildOf(Base);
}

void FInstancedStruct::Reset() {
    if (StructMemory != nullptr) {
        ScriptStruct->DestroyStruct(StructMemory);
        RuntimeFree(StructMemory);
    }
    StructMemory = nullptr;
    ScriptStruct = nullptr;
}

bool FInstancedStruct::operator==(const FInstancedStruct& Other) const {
    if (ScriptStruct != Other.ScriptStruct)
        return false;
    if (ScriptStruct == nullptr)
        return true;
    return ScriptStruct->CompareScriptStruct(StructMemory, Other.StructMemory);
}

//! Only an editor save that is not a cook writes the 0xABABABAB header of UE 5.0 to 5.2, so a cooked instance starts with the version byte.
bool FInstancedStruct::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FInstancedStructCustomVersion::GUID, FInstancedStructCustomVersion::CustomVersionAdded, EGame::GAME_UE5_3)) {
        uint8 Version = 0;
        Ar << Version;
        if (Version != 0) {
            Ar.SetError(ErrorCode::InvalidData, "FInstancedStruct.Version");
            return true;
        }
    }
    //! VEIN stores an instanced struct as a string.
    if (Ar.GetGame() == EGame::GAME_VEIN) {
        FString StringData;
        Ar << StringData;
        return true;
    }
    UObject* StructObject = nullptr;
    Ar << StructObject;
    int32 SerialSize = 0;
    Ar << SerialSize;
    if (Ar.IsError())
        return true;
    InitializeAs(Cast<UScriptStruct>(StructObject));
    SerializeStructInstance(Ar, ScriptStruct, StructMemory, SerialSize);
    return true;
}

}
