#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryDeprecated.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

UScriptStruct* SerializeScriptStruct(FArchive& Ar) {
    UObject* Object = nullptr;
    Ar << Object;
    return Cast<UScriptStruct>(Object);
}

void SerializeValues(FArchive& Ar, ERigVMRegisterType Type, FRigVMRegisterValues& Values) {
    switch (Type) {
    case ERigVMRegisterType::Plain:
        Ar << Values.Bytes;
        return;
    case ERigVMRegisterType::Name:
        Ar << Values.Names;
        return;
    case ERigVMRegisterType::String:
    case ERigVMRegisterType::Struct:
        Ar << Values.Strings;
        return;
    case ERigVMRegisterType::Invalid:
        Ar.SetError(ErrorCode::InvalidData, "FRigVMMemoryContainer.RegisterType");
        return;
    }
}

}

bool FRigVMRegister::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21))
        return false;
    uint16 SliceIndex = 0;
    Ar << Type << ByteIndex << ElementSize << ElementCount << SliceIndex << SliceCount << AlignmentBytes << TrailingBytes << Name << ScriptStructIndex;
    bIsArray = false;
    bIsDynamic = false;
    if (Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeRigVMRegisterArrayState, EGame::GAME_UE4_25))
        Ar << bIsArray;
    if (Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeRigVMRegisterDynamicState, EGame::GAME_UE4_26))
        Ar << bIsDynamic;
    return true;
}

bool FRigVMRegisterOffset::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21))
        return false;
    Ar << Segments << Type << CPPType;
    const bool bHasSegmentPath = Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::SerializeRigVMOffsetSegmentPaths, EGame::GAME_UE4_26);
    if (bHasSegmentPath)
        ScriptStruct = SerializeScriptStruct(Ar);
    else {
        FName ScriptStructPath;
        Ar << ScriptStructPath;
        ScriptStruct = Cast<UScriptStruct>(StaticFindObject(UScriptStruct::StaticClass(), nullptr, FString(ScriptStructPath.toString())));
    }
    Ar << ElementSize;
    if (!bHasSegmentPath)
        return true;
    ParentScriptStruct = SerializeScriptStruct(Ar);
    Ar << CachedSegmentPath << ArrayIndex;
    return true;
}

bool FRigVMMemoryContainer::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21))
        return false;
    Ar << bUseNameMap << MemoryType << Registers << RegisterOffsets << ScriptStructPaths;
    uint64 TotalBytes = 0;
    Ar << TotalBytes;
    RegisterValues.Empty(Registers.Num());
    for (const FRigVMRegister& Register : Registers) {
        TArray<FRigVMRegisterValues>& Slices = RegisterValues[RegisterValues.AddDefaulted()];
        if (Register.ElementCount == 0 && !Register.IsDynamic())
            continue;
        const int32 SliceCount = Register.IsNestedDynamic() ? Register.SliceCount : 1;
        for (int32 SliceIndex = 0; SliceIndex < SliceCount && !Ar.IsError(); SliceIndex++)
            SerializeValues(Ar, Register.Type, Slices[Slices.AddDefaulted()]);
    }
    return true;
}

}
