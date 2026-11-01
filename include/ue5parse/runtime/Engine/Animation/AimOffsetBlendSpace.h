#pragma once

#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "AimOffsetBlendSpace.generated.h"

namespace ue5parse::runtime {

// UAimOffsetBlendSpace (UE's Animation/AimOffsetBlendSpace.h).
UCLASS()
class UAimOffsetBlendSpace : public UBlendSpace {
    GENERATED_BODY()
public:
    explicit UAimOffsetBlendSpace(const FObjectInitializer& ObjectInitializer);
};

}
