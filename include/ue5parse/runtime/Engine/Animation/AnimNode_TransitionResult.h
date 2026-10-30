#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_TransitionResult.generated.h"

namespace ue5parse::runtime {

// The result of a transition rule's graph: its exposed value handler sets bCanEnterTransition (UE's
// Animation/AnimNode_TransitionResult.h).
USTRUCT()
struct FAnimNode_TransitionResult : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bCanEnterTransition = false;
};

}
