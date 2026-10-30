#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "CommonAnimTypes.generated.h"

namespace ue5parse::runtime {

// FAxis: an axis in local or component space (UE's AnimationCore CommonAnimTypes.h).
USTRUCT()
struct FAxis {
    GENERATED_BODY()
    UPROPERTY()
    FVector Axis = FVector::ForwardVector;
    UPROPERTY()
    bool bInLocalSpace = true;

    void Initialize() { Axis = Axis.GetSafeNormal(); }
    // The axis in the space of a transform: rotated by it when in local space.
    FVector GetTransformedAxis(const FTransform& Transform) const {
        return bInLocalSpace ? Transform.TransformVectorNoScale(Axis) : Axis;
    }
};

}
