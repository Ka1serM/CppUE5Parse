#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ShapeElem.h"

#include "ConvexElem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKConvexElem : public FKShapeElem {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FVector> VertexData;
    UPROPERTY()
    TArray<int32> IndexData;
    UPROPERTY()
    FBox ElemBox;
    UPROPERTY()
    FTransform Transform;
};

}
