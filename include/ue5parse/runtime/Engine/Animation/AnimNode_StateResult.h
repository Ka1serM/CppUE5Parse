#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_Root.h"

#include "AnimNode_StateResult.generated.h"

namespace ue5parse::runtime {

// The output pose of a state's graph (UE's Animation/AnimNode_StateResult.h). The state's entry, exit and blend
// functions are not part of the runtime.
USTRUCT()
struct FAnimNode_StateResult : public FAnimNode_Root {
    GENERATED_BODY()
};

}
