#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryCommon.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

enum class ERigVMOpCode : uint8 {
    Execute_0_Operands,
    Execute_64_Operands = 64,
    Zero,
    BoolFalse,
    BoolTrue,
    Copy,
    Increment,
    Decrement,
    Equals,
    NotEquals,
    JumpAbsolute,
    JumpForward,
    JumpBackward,
    JumpAbsoluteIf,
    JumpForwardIf,
    JumpBackwardIf,
    ChangeType,
    Exit,
    BeginBlock,
    EndBlock,
    Invalid,
};

//! The port keeps each instruction with its operands instead of the aligned byte code that UE builds from them.
struct FRigVMInstruction {
    ERigVMOpCode OpCode = ERigVMOpCode::Invalid;
    uint16 FunctionIndex = 0;
    TArray<FRigVMOperand> Operands;
    int32 InstructionIndex = -1;
    bool bCondition = false;
};

struct FRigVMByteCode {
    TArray<uint8> ByteCode;
    TArray<FRigVMInstruction> Instructions;
    TArray<FString> Entries;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMByteCode& ByteCode) {
        ByteCode.Serialize(Ar);
        return Ar;
    }
};

}
