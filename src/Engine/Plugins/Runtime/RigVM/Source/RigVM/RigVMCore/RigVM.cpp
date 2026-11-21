#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVM.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

bool HasDebugMap(FArchive& Ar) {
    return Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RigVMSaveDebugMapInGraphFunctionData, EGame::GAME_UE5_3)
           || Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::RigVMSaveDebugMapInGraphFunctionData,
                                  EGame::GAME_UE5_4);
}

void SerializePreviousDebugMap(FArchive& Ar) {
    if (!HasDebugMap(Ar) || FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::DebugOperandMappingSimplified))
        return;
    TMap<FRigVMOperand, TArray<FRigVMOperand>> PreviousOperandToDebugRegisters;
    Ar << PreviousOperandToDebugRegisters;
}

void SerializeUserDefinedTypeMaps(FArchive& Ar) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::HostStoringUserDefinedData))
        return;
    TMap<FString, FSoftObjectPath> UserDefinedStructGuidToPathName;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMStoringUserDefinedStructMap))
        Ar << UserDefinedStructGuidToPathName;
    TMap<FString, FSoftObjectPath> UserDefinedEnumToPathName;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMStoringUserDefinedEnumMap))
        Ar << UserDefinedEnumToPathName;
}

}

bool FRigVMParameter::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_20))
        return false;
    Ar << Type << Name << RegisterIndex << CPPType << ScriptStructPath;
    return true;
}

bool URigVM::SerializeUClassBasedStorage(FArchive& Ar) {
    int32 RigVMUClassBasedStorageDefine = 1;
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMMemoryStorageObject, EGame::GAME_UE5_0))
        Ar << RigVMUClassBasedStorageDefine;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMExternalExecuteContextStruct, EGame::GAME_UE5_3)
        && Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMSerializeExecuteContextStruct, EGame::GAME_UE5_2)) {
        FString ExecuteContextPath;
        Ar << ExecuteContextPath;
    }
    if (RigVMUClassBasedStorageDefine != 1)
        return RigVMUClassBasedStorageDefine == 0;
    Ar << WorkMemoryStorage << LiteralMemoryStorage << FunctionNamesStorage << ByteCodeStorage << Parameters;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMCopyOpStoreNumBytes, EGame::GAME_UE5_0))
        return false;
    if (HasDebugMap(Ar)) {
        TMap<FRigVMOperand, TArray<FRigVMOperand>> OperandToDebugRegisters;
        Ar << OperandToDebugRegisters;
    }
    SerializeUserDefinedTypeMaps(Ar);
    return false;
}

void URigVM::SerializeLocalizedRegistry(FArchive& Ar) {
    if (!FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::LocalizedRegistry))
        return;
    bool bStoredLocalizedRegistry = false;
    Ar << bStoredLocalizedRegistry;
    LocalizedRegistry.reset();
    if (!bStoredLocalizedRegistry || Ar.IsError())
        return;
    int64 RegistrySeekOffset = 0;
    Ar << RegistrySeekOffset << LocalizedRegistry.emplace();
    if (Ar.IsError())
        LocalizedRegistry.reset();
}

void URigVM::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_20))
        return;
    if (!FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::BeforeCustomVersionWasAdded) && !SerializeUClassBasedStorage(Ar))
        return;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::AddedVMHashChecks))
        Ar << CachedVMHash;
    CachedCanonicalVMHash = CachedVMHash;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::NativizedFunctionAsCallables))
        Ar << CachedCanonicalVMHash;
    Ar << ExternalPropertyPathDescriptions << FunctionNamesStorage << ByteCodeStorage << Parameters;
    SerializePreviousDebugMap(Ar);
    SerializeUserDefinedTypeMaps(Ar);
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMMemoryStorageStructSerialized))
        Ar << LiteralMemoryStorageStruct;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMMemoryStorageDefaultsGeneratedAtVM))
        Ar << DefaultWorkMemoryStorage << DefaultDebugMemoryStorage_DEPRECATED;
    SerializeLocalizedRegistry(Ar);
}

}
