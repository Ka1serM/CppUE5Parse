#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMRegistry.h"

namespace ue5parse::runtime {

namespace {

void SerializeFlattenedArgument(FArchive& Ar, FRigVMTemplateArgumentInfo& Argument) {
    Ar << Argument.Name;
    int32 Direction = 0;
    Ar << Direction;
    Argument.Direction = static_cast<ERigVMPinDirection>(Direction);
    const int32 NumTypes = Ar.ReadCount(12);
    Argument.Types.Empty(NumTypes);
    for (int32 TypeIndex = 0; TypeIndex < NumTypes && !Ar.IsError(); TypeIndex++)
        Argument.Types[Argument.Types.AddDefaulted()].Serialize_NoLock(Ar);
}

void SerializeFactory(FArchive& Ar, FRigVMDispatchFactory& Factory) {
    Ar << Factory.FactoryScriptStruct;
    const int32 NumFlattenedArguments = Ar.ReadCount(16);
    Factory.FlattenedArguments.Empty(NumFlattenedArguments);
    for (int32 ArgumentIndex = 0; ArgumentIndex < NumFlattenedArguments && !Ar.IsError(); ArgumentIndex++)
        SerializeFlattenedArgument(Ar, Factory.FlattenedArguments[Factory.FlattenedArguments.AddDefaulted()]);
}

}

void FRigVMRegistry_NoLock::Serialize_NoLock(FArchive& Ar) {
    Ar << AllowedClasses;
    const int32 NumTypes = Ar.ReadCount(12);
    Types.Empty(NumTypes);
    for (int32 TypeIndex = 0; TypeIndex < NumTypes && !Ar.IsError(); TypeIndex++)
        Types[Types.AddDefaulted()].Serialize_NoLock(Ar);
    const int32 NumFactories = Ar.ReadCount(8);
    Factories.Empty(NumFactories);
    for (int32 FactoryIndex = 0; FactoryIndex < NumFactories && !Ar.IsError(); FactoryIndex++)
        SerializeFactory(Ar, Factories[Factories.AddDefaulted()]);
    const int32 NumFunctions = Ar.ReadCount(16);
    Functions.Empty(NumFunctions);
    for (int32 FunctionIndex = 0; FunctionIndex < NumFunctions && !Ar.IsError(); FunctionIndex++)
        Functions[Functions.AddDefaulted()].Serialize_NoLock(Ar);
}

}
