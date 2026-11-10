#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "CommonAnimTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAxis {
    GENERATED_BODY()
    UPROPERTY()
    FVector Axis = FVector::ForwardVector;
    UPROPERTY()
    bool bInLocalSpace = true;

    void Initialize() { Axis = Axis.GetSafeNormal(); }
    FVector GetTransformedAxis(const FTransform& Transform) const { return bInLocalSpace ? Transform.TransformVectorNoScale(Axis) : Axis; }
};

}
