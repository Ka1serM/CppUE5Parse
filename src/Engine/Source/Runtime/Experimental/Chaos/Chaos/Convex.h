#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ConvexStructureData.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Plane.h"

namespace ue5parse::runtime::Chaos {

class FConvex {
public:
    void Serialize(FArchive& Ar);

    const TArray<FPlaneConcrete3f>& GetPlanes() const { return Planes; }
    const TArray<FVector3f>& GetVertices() const { return Vertices; }
    const FAABB3f& GetLocalBoundingBox() const { return LocalBoundingBox; }
    const FConvexStructureData& GetStructureData() const { return StructureData; }
    float GetVolume() const { return Volume; }
    const FVector3f& GetCenterOfMass() const { return CenterOfMass; }
    float GetMargin() const { return Margin; }
    const FVector3f& GetUnitMassInertiaTensor() const { return UnitMassInertiaTensor; }
    const FQuat& GetRotationOfMass() const { return RotationOfMass; }

private:
    TArray<FPlaneConcrete3f> Planes;
    TArray<FVector3f> Vertices;
    FAABB3f LocalBoundingBox;
    FConvexStructureData StructureData;
    float Volume = 0.0f;
    FVector3f CenterOfMass;
    float Margin = 0.0f;
    FVector3f UnitMassInertiaTensor;
    FQuat RotationOfMass;
};

}
