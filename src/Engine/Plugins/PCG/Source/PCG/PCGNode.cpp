#include "Engine/Plugins/PCG/Source/PCG/PCGNode.h"

#include "Engine/Plugins/PCG/Source/PCG/PCGSettings.h"

namespace ue5parse::runtime {

//! 5.1 stores this default subobject in DefaultSettings instead of SettingsInterface.
UPCGNode::UPCGNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SettingsInterface = ObjectInitializer.CreateDefaultSubobject<UPCGTrivialSettings>(FName("DefaultNodeSettings"));
}

}
