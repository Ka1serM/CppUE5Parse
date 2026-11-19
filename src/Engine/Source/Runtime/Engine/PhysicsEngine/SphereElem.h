#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ShapeElem.h"

#include "SphereElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKSphereElem : public FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    FVector Center = FVector::ZeroVector;
    UPROPERTY()
    float Radius = 1.0f;
};

}
