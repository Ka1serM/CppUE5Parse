#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMByteCode.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMExternalVariable.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionIdentifier.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMNodeLayout.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectArchive.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMVariant.h"

#include "RigVMGraphFunctionDefinition.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMFunctionCompilationPropertyDescription {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FString CPPType;
    UPROPERTY()
    TSoftObjectPtr<UObject> CPPTypeObject;
    UPROPERTY()
    FString DefaultValue;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationPropertyDescription& Data) {
        return Ar << Data.Name << Data.CPPType << Data.CPPTypeObject << Data.DefaultValue;
    }
};

USTRUCT()
struct FRigVMFunctionCompilationPropertyPath {
    GENERATED_BODY()
    UPROPERTY()
    int32 PropertyIndex = -1;
    UPROPERTY()
    FString HeadCPPType;
    UPROPERTY()
    FString SegmentPath;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationPropertyPath& Data) { return Ar << Data.PropertyIndex << Data.HeadCPPType << Data.SegmentPath; }
};

USTRUCT()
struct FRigVMFunctionCompilationData {
    GENERATED_BODY()
    FRigVMByteCode ByteCode;
    UPROPERTY()
    TArray<FName> FunctionNames;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyDescription> WorkPropertyDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyPath> WorkPropertyPathDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyDescription> LiteralPropertyDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyPath> LiteralPropertyPathDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyDescription> DebugPropertyDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyPath> DebugPropertyPathDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyDescription> ExternalPropertyDescriptions;
    UPROPERTY()
    TArray<FRigVMFunctionCompilationPropertyPath> ExternalPropertyPathDescriptions;
    UPROPERTY()
    TMap<int32, FName> ExternalRegisterIndexToVariable;
    TMap<FString, FRigVMOperand> Operands;
    TMap<FName, FRigVMOperand> InterfaceOperands;
    UPROPERTY()
    uint32 Hash = 0;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationData& Data);
};

USTRUCT()
struct FRigVMGraphFunctionArgument {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName DisplayName;
    UPROPERTY()
    FName CPPType;
    UPROPERTY()
    TSoftObjectPtr<UObject> CPPTypeObject;
    UPROPERTY()
    bool bIsArray = false;
    ERigVMPinDirection Direction = ERigVMPinDirection::Input;
    UPROPERTY()
    FString DefaultValue;
    UPROPERTY()
    bool bIsConst = false;
    UPROPERTY()
    bool bIsInputVariable = false;
    UPROPERTY()
    TMap<FString, FText> PathToTooltip;

    void Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionArgument& Data) {
        Data.Serialize(Ar);
        return Ar;
    }
};

USTRUCT()
struct FRigVMGraphFunctionHeader {
    GENERATED_BODY()
    UPROPERTY()
    FRigVMGraphFunctionIdentifier LibraryPointer;
    UPROPERTY()
    FRigVMVariant Variant;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FString NodeTitle;
    UPROPERTY()
    FLinearColor NodeColor = FLinearColor::White;
    UPROPERTY()
    FText Tooltip_DEPRECATED;
    UPROPERTY()
    FString Description;
    UPROPERTY()
    FString Category;
    UPROPERTY()
    FString Keywords;
    UPROPERTY()
    TArray<FRigVMGraphFunctionArgument> Arguments;
    UPROPERTY()
    TMap<FRigVMGraphFunctionIdentifier, uint32> Dependencies;
    //! UE 5.8 drops the stored variables without a name or a type after loading; the port keeps every stored variable.
    UPROPERTY()
    TArray<FRigVMExternalVariable> ExternalVariables;
    UPROPERTY()
    FRigVMNodeLayout Layout;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionHeader& Data);
};

USTRUCT()
struct FRigVMGraphFunctionData {
    GENERATED_BODY()
    UPROPERTY()
    FRigVMGraphFunctionHeader Header;
    UPROPERTY()
    FRigVMFunctionCompilationData CompilationData;
    UPROPERTY()
    FString SerializedCollapsedNode_DEPRECATED;
    UPROPERTY()
    FRigVMObjectArchive CollapseNodeArchive;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionData& Data);
};

}
