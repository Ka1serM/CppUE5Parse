#pragma once

#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "BlendSpace1D.generated.h"

namespace ue5parse::runtime {

// UBlendSpace1D (UE's Animation/BlendSpace1D.h).
UCLASS()
class UBlendSpace1D : public UBlendSpace {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bScaleAnimation = false;

    explicit UBlendSpace1D(const FObjectInitializer& ObjectInitializer);
    EBlendSpaceAxis GetAxisToScale() const override { return bScaleAnimation ? BSA_X : BSA_None; }
};

}
