#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMFunctionCompilationData.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationData& Data) {
    Ar << Data.ByteCode << Data.FunctionNames;
    Ar << Data.WorkPropertyDescriptions << Data.WorkPropertyPathDescriptions;
    Ar << Data.LiteralPropertyDescriptions << Data.LiteralPropertyPathDescriptions;
    Ar << Data.DebugPropertyDescriptions << Data.DebugPropertyPathDescriptions;
    Ar << Data.ExternalPropertyDescriptions << Data.ExternalPropertyPathDescriptions;
    Ar << Data.ExternalRegisterIndexToVariable << Data.Operands;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RigVMCallables))
        Ar << Data.InterfaceOperands;
    Ar << Data.Hash;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::DebugOperandMappingSimplified))
        return Ar;
    if (!Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RigVMSaveDebugMapInGraphFunctionData, EGame::GAME_UE5_3)
        && !Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::RigVMSaveDebugMapInGraphFunctionData, EGame::GAME_UE5_4))
        return Ar;
    uint8 NumKeys = 0;
    Ar << NumKeys;
    for (int32 Index = 0; Index < NumKeys && !Ar.IsError(); ++Index) {
        FRigVMOperand Key;
        uint8 NumValues = 0;
        Ar << Key << NumValues;
        auto& Values = Data.OperandToDebugRegisters.Add(Key);
        Values.SetNum(NumValues);
        for (auto& Operand : Values)
            Ar << Operand;
    }
    return Ar;
}

}
