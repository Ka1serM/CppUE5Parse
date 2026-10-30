#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_Root.h"

#include "AnimNode_BlendSpaceSampleResult.generated.h"

namespace ue5parse::runtime {

// The output of a blend space graph's sample graph (UE's AnimNodes/AnimNode_BlendSpaceSampleResult.h).
USTRUCT()
struct FAnimNode_BlendSpaceSampleResult : public FAnimNode_Root {
    GENERATED_BODY()
};

}
