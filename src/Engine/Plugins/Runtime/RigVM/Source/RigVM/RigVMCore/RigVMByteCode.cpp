#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

bool IsExecuteOpCode(ERigVMOpCode OpCode) {
    return OpCode <= ERigVMOpCode::Execute_64_Operands;
}

void SerializeOperands(FArchive& Ar, FRigVMInstruction& Instruction, int32 Count) {
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << Instruction.Operands[Instruction.Operands.AddDefaulted()];
}

void SerializeCallableOp(FArchive& Ar, FRigVMInstruction& Instruction, bool bHasPredicates) {
    ERigVMOpCode StoredOpCode = ERigVMOpCode::Invalid;
    Ar << StoredOpCode << Instruction.CallableIndex;
    uint16 ArgumentCount = static_cast<uint16>(StoredOpCode);
    if (!IsExecuteOpCode(StoredOpCode))
        Ar << ArgumentCount;
    if (bHasPredicates && FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::PredicatesAddedToExecuteOps))
        Ar << Instruction.FirstPredicateIndex << Instruction.PredicateCount;
    SerializeOperands(Ar, Instruction, ArgumentCount);
}

void SerializeOperandOp(FArchive& Ar, FRigVMInstruction& Instruction, int32 Count) {
    ERigVMOpCode StoredOpCode = ERigVMOpCode::Invalid;
    Ar << StoredOpCode;
    SerializeOperands(Ar, Instruction, Count);
}

void SerializeCopyOp(FArchive& Ar, FRigVMInstruction& Instruction) {
    SerializeOperandOp(Ar, Instruction, 2);
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMCopyOpStoreNumBytes, EGame::GAME_UE5_0))
        return;
    if (!FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::ByteCodeCleanup)) {
        uint16 NumBytes = 0;
        Ar << NumBytes;
    }
    Ar << Instruction.RegisterType;
}

void SerializeInstruction(FArchive& Ar, FRigVMInstruction& Instruction) {
    Ar << Instruction.OpCode;
    if (IsExecuteOpCode(Instruction.OpCode))
        Instruction.OpCode = ERigVMOpCode::Execute;
    switch (Instruction.OpCode) {
    case ERigVMOpCode::Execute:
        SerializeCallableOp(Ar, Instruction, true);
        return;
    case ERigVMOpCode::InvokeCallable:
        SerializeCallableOp(Ar, Instruction, false);
        return;
    case ERigVMOpCode::Copy:
        SerializeCopyOp(Ar, Instruction);
        return;
    case ERigVMOpCode::Zero:
    case ERigVMOpCode::BoolFalse:
    case ERigVMOpCode::BoolTrue:
    case ERigVMOpCode::Increment:
    case ERigVMOpCode::Decrement:
    case ERigVMOpCode::ArrayReset:
    case ERigVMOpCode::ArrayReverse:
    case ERigVMOpCode::SetupTraits:
        SerializeOperandOp(Ar, Instruction, 1);
        return;
    case ERigVMOpCode::Equals:
    case ERigVMOpCode::NotEquals:
        SerializeOperandOp(Ar, Instruction, 3);
        return;
    case ERigVMOpCode::JumpAbsolute:
    case ERigVMOpCode::JumpForward:
    case ERigVMOpCode::JumpBackward:
        SerializeOperandOp(Ar, Instruction, 0);
        Ar << Instruction.InstructionIndex;
        return;
    case ERigVMOpCode::JumpAbsoluteIf:
    case ERigVMOpCode::JumpForwardIf:
    case ERigVMOpCode::JumpBackwardIf:
        SerializeOperandOp(Ar, Instruction, 1);
        Ar << Instruction.InstructionIndex << Instruction.bCondition;
        return;
    case ERigVMOpCode::Exit:
    case ERigVMOpCode::EndBlock:
        return;
    case ERigVMOpCode::BeginBlock:
    case ERigVMOpCode::ArrayGetNum:
    case ERigVMOpCode::ArraySetNum:
    case ERigVMOpCode::ArrayAppend:
    case ERigVMOpCode::ArrayClone:
    case ERigVMOpCode::ArrayRemove:
    case ERigVMOpCode::ArrayUnion:
        SerializeOperandOp(Ar, Instruction, 2);
        return;
    case ERigVMOpCode::ArrayAdd:
    case ERigVMOpCode::ArrayGetAtIndex:
    case ERigVMOpCode::ArraySetAtIndex:
    case ERigVMOpCode::ArrayInsert:
    case ERigVMOpCode::ArrayDifference:
    case ERigVMOpCode::ArrayIntersection:
        SerializeOperandOp(Ar, Instruction, 3);
        return;
    case ERigVMOpCode::ArrayFind:
        SerializeOperandOp(Ar, Instruction, 4);
        return;
    case ERigVMOpCode::ArrayIterator:
        SerializeOperandOp(Ar, Instruction, 6);
        return;
    case ERigVMOpCode::InvokeEntry: {
        FString EntryName;
        Ar << EntryName;
        Instruction.EntryName = MakeName(EntryName);
        return;
    }
    case ERigVMOpCode::JumpToBranch:
        SerializeOperandOp(Ar, Instruction, 1);
        Ar << Instruction.FirstBranchInfoIndex;
        return;
    case ERigVMOpCode::RunInstructions:
        SerializeOperandOp(Ar, Instruction, 1);
        Ar << Instruction.StartInstruction << Instruction.EndInstruction;
        return;
    default:
        Ar.SetError(ErrorCode::InvalidData, "FRigVMByteCode.OpCode");
        return;
    }
}

}

void FRigVMBranchInfo::Serialize(FArchive& Ar) {
    FString LabelString;
    Ar << Index << LabelString << InstructionIndex << ArgumentIndex;
    Label = MakeName(LabelString);
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::ByteCodeCleanup)) {
        Ar << FirstInstruction << LastInstruction;
        return;
    }
    uint16 FirstInstructionTruncated = 0;
    uint16 LastInstructionTruncated = 0;
    Ar << FirstInstructionTruncated << LastInstructionTruncated;
    FirstInstruction = FirstInstructionTruncated;
    LastInstruction = LastInstructionTruncated;
}

bool FRigVMByteCode::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_20))
        return false;
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::RigVMByteCodeDeterminism, EGame::GAME_UE4_26)) {
        Ar << ByteCode;
        return true;
    }
    int32 InstructionCount = 0;
    Ar << InstructionCount;
    Instructions.Empty();
    for (int32 Index = 0; Index < InstructionCount && !Ar.IsError(); Index++)
        SerializeInstruction(Ar, Instructions[Instructions.AddDefaulted()]);
    if (Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::SerializeRigVMEntries, EGame::GAME_UE4_26))
        Ar << Entries;
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMLazyEvaluation, EGame::GAME_UE5_2))
        Ar << BranchInfos;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RigVMCallables))
        Ar << CallableInfos;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMBytecodeStorePublicContextPathAsTopLevelAssetPath)) {
        Ar << PublicContextAssetPath.PackageName << PublicContextAssetPath.AssetName;
        return true;
    }
    if (!FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMBytecodeStorePublicContextPath))
        return true;
    FString PublicContextPathName;
    Ar << PublicContextPathName;
    PublicContextAssetPath = FSoftObjectPath(PublicContextPathName).AssetPath;
    return true;
}

}
