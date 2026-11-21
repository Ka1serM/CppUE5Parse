#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse::runtime {

struct FRigVMTemplateArgumentType {
    FName CPPType;
    FObjectPtr CPPTypeObject;

    friend FArchive& operator<<(FArchive& Ar, FRigVMTemplateArgumentType& Type) { return Ar << Type.CPPType << Type.CPPTypeObject; }
};

struct FRigVMTemplateArgumentInfo {
    FName ArgumentName;
    int32 Direction = 0;
    TArray<FRigVMTemplateArgumentType> Types;

    friend FArchive& operator<<(FArchive& Ar, FRigVMTemplateArgumentInfo& Argument) { return Ar << Argument.ArgumentName << Argument.Direction << Argument.Types; }
};

struct FRigVMDispatchFactory {
    FObjectPtr FactoryScriptStruct;
    TArray<FRigVMTemplateArgumentInfo> FlattenedArguments;

    friend FArchive& operator<<(FArchive& Ar, FRigVMDispatchFactory& Factory) { return Ar << Factory.FactoryScriptStruct << Factory.FlattenedArguments; }
};

struct FRigVMFunctionArgument {
    FString Name;
    FString Type;
    int32 Direction = 0;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunctionArgument& Argument) { return Ar << Argument.Name << Argument.Type << Argument.Direction; }
};

struct FRigVMFunction {
    FString Name;
    FObjectPtr Struct;
    FObjectPtr FactoryStruct;
    TArray<FRigVMFunctionArgument> Arguments;

    friend FArchive& operator<<(FArchive& Ar, FRigVMFunction& Function) { return Ar << Function.Name << Function.Struct << Function.FactoryStruct << Function.Arguments; }
};

struct FRigVMRegistry_NoLock {
    TArray<FObjectPtr> AllowedClasses;
    TArray<FRigVMTemplateArgumentType> Types;
    TArray<FRigVMDispatchFactory> Factories;
    TArray<FRigVMFunction> Functions;

    friend FArchive& operator<<(FArchive& Ar, FRigVMRegistry_NoLock& Registry) {
        return Ar << Registry.AllowedClasses << Registry.Types << Registry.Factories << Registry.Functions;
    }
};

}
