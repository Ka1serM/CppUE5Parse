#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMFunction.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryCommon.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryDeprecated.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

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
    ArrayReset,
    ArrayGetNum,
    ArraySetNum,
    ArrayGetAtIndex,
    ArraySetAtIndex,
    ArrayAdd,
    ArrayInsert,
    ArrayRemove,
    ArrayFind,
    ArrayAppend,
    ArrayClone,
    ArrayIterator,
    ArrayUnion,
    ArrayDifference,
    ArrayIntersection,
    ArrayReverse,
    InvokeEntry,
    JumpToBranch,
    Execute,
    RunInstructions,
    SetupTraits,
    InvokeCallable,
    Invalid,
};

//! The port keeps each instruction with its operands instead of the aligned byte code that UE builds from them.
struct FRigVMInstruction {
    ERigVMOpCode OpCode = ERigVMOpCode::Invalid;
    uint16 CallableIndex = 0;
    uint16 FirstPredicateIndex = 0;
    uint16 PredicateCount = 0;
    TArray<FRigVMOperand> Operands;
    ERigVMRegisterType RegisterType = ERigVMRegisterType::Invalid;
    int32 InstructionIndex = -1;
    bool bCondition = false;
    int32 FirstBranchInfoIndex = -1;
    int32 StartInstruction = -1;
    int32 EndInstruction = -1;
    FName EntryName;
};

struct FRigVMBranchInfo {
    int32 Index = -1;
    FName Label;
    int32 InstructionIndex = -1;
    int32 ArgumentIndex = -1;
    int32 FirstInstruction = -1;
    int32 LastInstruction = -1;

    void Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMBranchInfo& Info) {
        Info.Serialize(Ar);
        return Ar;
    }
};

struct FRigVMCallableArgument {
    FName Name;
    FString TypeString;
    FRigVMOperand InterfaceOperand;
    FRigVMOperand ForwardedOperand;
    ERigVMPinDirection Direction = ERigVMPinDirection::Invalid;

    friend FArchive& operator<<(FArchive& Ar, FRigVMCallableArgument& Argument) {
        return Ar << Argument.Name << Argument.TypeString << Argument.InterfaceOperand << Argument.ForwardedOperand << Argument.Direction;
    }
};

struct FRigVMCallableInfo {
    int32 Index = -1;
    FName Name;
    uint32 FunctionHash = 0;
    TArray<FRigVMCallableArgument> Arguments;
    int32 FirstInstruction = -1;
    int32 LastInstruction = -1;

    friend FArchive& operator<<(FArchive& Ar, FRigVMCallableInfo& Info) {
        return Ar << Info.Index << Info.Name << Info.FunctionHash << Info.Arguments << Info.FirstInstruction << Info.LastInstruction;
    }
};

struct FRigVMByteCode {
    TArray<uint8> ByteCode;
    TArray<FRigVMInstruction> Instructions;
    //! UE imports each entry from text into an FRigVMByteCodeEntry; the port keeps the text.
    TArray<FString> Entries;
    TArray<FRigVMBranchInfo> BranchInfos;
    TArray<FRigVMCallableInfo> CallableInfos;
    FTopLevelAssetPath PublicContextAssetPath;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMByteCode& ByteCode) {
        ByteCode.Serialize(Ar);
        return Ar;
    }
};

}
