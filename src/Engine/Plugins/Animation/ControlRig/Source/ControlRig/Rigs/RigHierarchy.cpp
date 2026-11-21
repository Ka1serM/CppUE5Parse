#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchy.h"

namespace ue5parse::runtime {

void URigHierarchy::Serialize(FArchive& Ar) {
    Hierarchy.Load(Ar);
}

}
