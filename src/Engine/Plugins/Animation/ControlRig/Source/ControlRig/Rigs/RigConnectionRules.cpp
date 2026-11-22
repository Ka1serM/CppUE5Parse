#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigConnectionRules.h"

namespace ue5parse::runtime {

void FRigConnectionRuleStash::Load(FArchive& Ar) {
    Ar << ScriptStructPath;
    Ar << ExportedText;
}

}
