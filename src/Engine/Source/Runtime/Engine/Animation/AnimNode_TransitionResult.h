#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_TransitionResult.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_TransitionResult : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bCanEnterTransition = false;
};

}
