#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ShapeElem.h"

#include "TaperedCapsuleElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKTaperedCapsuleElem : public FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    FVector Center = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY()
    float Radius0 = 1.0f;
    UPROPERTY()
    float Radius1 = 1.0f;
    UPROPERTY()
    float Length = 1.0f;
    UPROPERTY()
    float Width = 0.0f;
    UPROPERTY()
    bool bOneSidedCollision = false;
};

}
