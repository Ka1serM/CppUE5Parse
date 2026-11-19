#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/BoxElem.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConvexElem.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/SphereElem.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/SphylElem.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/TaperedCapsuleElem.h"

#include "AggregateGeom.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKAggregateGeom {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FKSphereElem> SphereElems;
    UPROPERTY()
    TArray<FKBoxElem> BoxElems;
    UPROPERTY()
    TArray<FKSphylElem> SphylElems;
    UPROPERTY()
    TArray<FKConvexElem> ConvexElems;
    UPROPERTY()
    TArray<FKTaperedCapsuleElem> TaperedCapsuleElems;
};

}
