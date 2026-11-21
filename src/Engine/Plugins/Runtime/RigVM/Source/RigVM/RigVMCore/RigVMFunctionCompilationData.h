#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMPropertyPath.h"

namespace ue5parse::runtime {

struct FRigVMFunctionCompilationPropertyDescription {
    FName Name;
    FString CPPType;
    FSoftObjectPath CPPTypeObject;
    FString DefaultValue;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationPropertyDescription& Property) {
        return Ar << Property.Name << Property.CPPType << Property.CPPTypeObject << Property.DefaultValue;
    }
};

struct FRigVMFunctionCompilationData {
    FRigVMByteCode ByteCode;
    TArray<FName> FunctionNames;
    TArray<FRigVMFunctionCompilationPropertyDescription> WorkPropertyDescriptions;
    TArray<FRigVMPropertyPathDescription> WorkPropertyPathDescriptions;
    TArray<FRigVMFunctionCompilationPropertyDescription> LiteralPropertyDescriptions;
    TArray<FRigVMPropertyPathDescription> LiteralPropertyPathDescriptions;
    TArray<FRigVMFunctionCompilationPropertyDescription> DebugPropertyDescriptions;
    TArray<FRigVMPropertyPathDescription> DebugPropertyPathDescriptions;
    TArray<FRigVMFunctionCompilationPropertyDescription> ExternalPropertyDescriptions;
    TArray<FRigVMPropertyPathDescription> ExternalPropertyPathDescriptions;
    TMap<int32, FName> ExternalRegisterIndexToVariable;
    TMap<FString, FRigVMOperand> Operands;
    TMap<FName, FRigVMOperand> InterfaceOperands;
    uint32 Hash = 0;
    TMap<FRigVMOperand, TArray<FRigVMOperand>> OperandToDebugRegisters;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationData& Data);
};

}
