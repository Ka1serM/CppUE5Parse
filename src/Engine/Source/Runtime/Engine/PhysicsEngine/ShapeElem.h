#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "ShapeElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    float RestOffset = 0.0f;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    bool bContributeToMass = true;
    UPROPERTY()
    bool bIsGenerated = false;
    UPROPERTY()
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
};

}
