#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ShapeElem.h"

#include "BoxElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKBoxElem : public FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    FVector Center = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    //! UE 4.3 to 4.15 store the rotation as a quaternion.
    UPROPERTY()
    FQuat Orientation = FQuat::Identity;
    UPROPERTY()
    float X = 1.0f;
    UPROPERTY()
    float Y = 1.0f;
    UPROPERTY()
    float Z = 1.0f;
};

}
