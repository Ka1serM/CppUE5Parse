#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMFunction.h"

namespace ue5parse::runtime {

void FRigVMFunctionArgument::Serialize_NoLock(FArchive& Ar) {
    Ar << Name << Type;
    int32 DirectionAsInt32 = 0;
    Ar << DirectionAsInt32;
    Direction = static_cast<ERigVMFunctionArgumentDirection>(DirectionAsInt32);
}

void FRigVMFunction::Serialize_NoLock(FArchive& Ar) {
    Ar << Name << Struct << FactoryStruct;
    const int32 NumArguments = Ar.ReadCount(12);
    Arguments.Empty(NumArguments);
    for (int32 ArgumentIndex = 0; ArgumentIndex < NumArguments && !Ar.IsError(); ArgumentIndex++)
        Arguments[Arguments.AddDefaulted()].Serialize_NoLock(Ar);
}

}
