#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMTemplate.h"

namespace ue5parse::runtime {

struct FRigVMDispatchFactory {
    TObjectPtr<UScriptStruct> FactoryScriptStruct;
    //! UE registers these arguments in the factory's template; the port keeps them on the factory.
    TArray<FRigVMTemplateArgumentInfo> FlattenedArguments;
};

}
