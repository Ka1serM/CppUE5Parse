#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

void SerializeOperands(FArchive& Ar, FRigVMInstruction& Instruction, int32 Count) {
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << Instruction.Operands[Instruction.Operands.AddDefaulted()];
}

void SerializeInstruction(FArchive& Ar, FRigVMInstruction& Instruction) {
    Ar << Instruction.OpCode;
    const ERigVMOpCode OpCode = Instruction.OpCode;
    if (OpCode == ERigVMOpCode::Exit || OpCode == ERigVMOpCode::EndBlock)
        return;
    ERigVMOpCode StoredOpCode = ERigVMOpCode::Invalid;
    Ar << StoredOpCode;
    if (OpCode <= ERigVMOpCode::Execute_64_Operands) {
        Ar << Instruction.FunctionIndex;
        SerializeOperands(Ar, Instruction, static_cast<int32>(StoredOpCode) - static_cast<int32>(ERigVMOpCode::Execute_0_Operands));
        return;
    }
    switch (OpCode) {
    case ERigVMOpCode::Zero:
    case ERigVMOpCode::BoolFalse:
    case ERigVMOpCode::BoolTrue:
    case ERigVMOpCode::Increment:
    case ERigVMOpCode::Decrement:
        SerializeOperands(Ar, Instruction, 1);
        return;
    case ERigVMOpCode::Copy:
    case ERigVMOpCode::BeginBlock:
        SerializeOperands(Ar, Instruction, 2);
        return;
    case ERigVMOpCode::Equals:
    case ERigVMOpCode::NotEquals:
        SerializeOperands(Ar, Instruction, 3);
        return;
    case ERigVMOpCode::JumpAbsolute:
    case ERigVMOpCode::JumpForward:
    case ERigVMOpCode::JumpBackward:
        Ar << Instruction.InstructionIndex;
        return;
    case ERigVMOpCode::JumpAbsoluteIf:
    case ERigVMOpCode::JumpForwardIf:
    case ERigVMOpCode::JumpBackwardIf:
        SerializeOperands(Ar, Instruction, 1);
        Ar << Instruction.InstructionIndex << Instruction.bCondition;
        return;
    default:
        Ar.SetError(ErrorCode::InvalidData, "FRigVMByteCode.OpCode");
        return;
    }
}

}

bool FRigVMByteCode::Serialize(FArchive& Ar) {
    if (Ar.CustomVer(FAnimObjectVersion::GUID) < FAnimObjectVersion::StoreMarkerNamesOnSkeleton)
        return false;
    if (Ar.CustomVer(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::RigVMByteCodeDeterminism) {
        Ar << ByteCode;
        return true;
    }
    int32 InstructionCount = 0;
    Ar << InstructionCount;
    Instructions.Empty();
    for (int32 Index = 0; Index < InstructionCount && !Ar.IsError(); Index++)
        SerializeInstruction(Ar, Instructions[Instructions.AddDefaulted()]);
    if (Ar.CustomVer(FAnimObjectVersion::GUID) >= FAnimObjectVersion::SerializeRigVMEntries)
        Ar << Entries;
    return true;
}

}
