#pragma once

#include "Engine/Source/Runtime/Engine/Animation/BlendSpace.h"

#include "AimOffsetBlendSpace.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAimOffsetBlendSpace : public UBlendSpace {
    GENERATED_BODY()
public:
    explicit UAimOffsetBlendSpace(const FObjectInitializer& ObjectInitializer);
};

}
