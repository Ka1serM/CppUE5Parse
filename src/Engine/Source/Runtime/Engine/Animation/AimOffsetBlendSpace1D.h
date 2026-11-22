#pragma once

#include "Engine/Source/Runtime/Engine/Animation/BlendSpace1D.h"

#include "AimOffsetBlendSpace1D.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAimOffsetBlendSpace1D : public UBlendSpace1D {
    GENERATED_BODY()
public:
    explicit UAimOffsetBlendSpace1D(const FObjectInitializer& ObjectInitializer);
};

}
