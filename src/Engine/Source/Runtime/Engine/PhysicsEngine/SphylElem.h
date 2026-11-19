#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ShapeElem.h"

#include "SphylElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKSphylElem : public FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    FVector Center = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    //! UE 4.3 to 4.15 store the rotation as a quaternion.
    UPROPERTY()
    FQuat Orientation = FQuat::Identity;
    UPROPERTY()
    float Radius = 1.0f;
    UPROPERTY()
    float Length = 1.0f;
};

}
