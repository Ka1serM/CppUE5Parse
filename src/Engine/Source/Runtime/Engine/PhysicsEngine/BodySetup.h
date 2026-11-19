#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/AggregateGeom.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"
#include "Engine/Source/Runtime/PhysicsCore/BodySetupCore.h"

#include "BodySetup.generated.h"

namespace ue5parse::runtime {

class UPhysicalMaterial;

UCLASS()
class UBodySetup : public UBodySetupCore {
    GENERATED_BODY()
public:
    UPROPERTY()
    FKAggregateGeom AggGeom;
    UPROPERTY()
    bool bConsiderForBounds = true;
    UPROPERTY()
    bool bMeshCollideAll = false;
    UPROPERTY()
    bool bDoubleSidedGeometry = false;
    UPROPERTY()
    bool bGenerateNonMirroredCollision = true;
    UPROPERTY()
    bool bSharedCookedData = false;
    UPROPERTY()
    bool bGenerateMirroredCollision = true;
    UPROPERTY()
    bool bSupportUVsAndFaceRemap = false;
    UPROPERTY()
    bool bSupportFaceRemapOnMeshBVH = false;
    UPROPERTY()
    bool bSupportVertexRemap = false;
    UPROPERTY()
    bool bNeverNeedsCookedCollisionData = false;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;
    UPROPERTY()
    FWalkableSlopeOverride WalkableSlopeOverride;
    UPROPERTY()
    FBodyInstance DefaultInstance;
    UPROPERTY()
    FVector BuildScale3D = FVector::OneVector;

    FGuid BodySetupGuid;
    bool bHasCookedCollisionData = true;
    FFormatContainer CookedFormatData;

    void Serialize(FArchive& Ar) override;
};

}
