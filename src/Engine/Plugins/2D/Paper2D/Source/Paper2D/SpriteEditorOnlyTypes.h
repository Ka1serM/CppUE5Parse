#pragma once

#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "SpriteEditorOnlyTypes.generated.h"

namespace ue5parse::runtime {

namespace ESpriteCollisionMode {

UENUM()
enum Type : int32 { None, Use2DPhysics, Use3DPhysics };

}

UENUM()
enum class ESpriteShapeType : uint8 { Box, Circle, Polygon };

USTRUCT()
struct FSpriteGeometryShape {
    GENERATED_BODY()
    UPROPERTY()
    ESpriteShapeType ShapeType = ESpriteShapeType::Box;
    UPROPERTY()
    TArray<FVector2D> Vertices;
    UPROPERTY()
    FVector2D BoxSize = FVector2D::ZeroVector;
    UPROPERTY()
    FVector2D BoxPosition = FVector2D::ZeroVector;
    UPROPERTY()
    float Rotation = 0.0f;
    UPROPERTY()
    bool bNegativeWinding = false;
};

namespace ESpritePolygonMode {

UENUM()
enum Type : int32 { SourceBoundingBox, TightBoundingBox, ShrinkWrapped, FullyCustom, Diced };

}

USTRUCT()
struct FSpriteGeometryCollection {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FSpriteGeometryShape> Shapes;
    UPROPERTY()
    TEnumAsByte<ESpritePolygonMode::Type> GeometryType = ESpritePolygonMode::TightBoundingBox;
    UPROPERTY()
    int32 PixelsPerSubdivisionX = 32;
    UPROPERTY()
    int32 PixelsPerSubdivisionY = 32;
    UPROPERTY()
    bool bAvoidVertexMerging = false;
    UPROPERTY()
    float AlphaThreshold = 0.0f;
    UPROPERTY()
    float DetailAmount = 0.5f;
    UPROPERTY()
    float SimplifyEpsilon = 2.0f;
};

}
