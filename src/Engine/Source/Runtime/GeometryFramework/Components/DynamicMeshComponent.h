#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/AggregateGeom.h"
#include "Engine/Source/Runtime/GeometryFramework/Components/BaseDynamicMeshComponent.h"
#include "Engine/Source/Runtime/PhysicsCore/BodySetupEnums.h"

#include "DynamicMeshComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UDynamicMesh;

UCLASS()
class UDynamicMeshComponent : public UBaseDynamicMeshComponent {
    GENERATED_BODY()
public:
    explicit UDynamicMeshComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDynamicMesh> MeshObject = nullptr;
    UPROPERTY()
    bool bAllowsGeometrySelection = true;
    UPROPERTY()
    EDynamicMeshComponentTangentsMode TangentsType = EDynamicMeshComponentTangentsMode::NoTangents;
    UPROPERTY()
    TEnumAsByte<ECollisionTraceFlag> CollisionType = CTF_UseSimpleAsComplex;
    UPROPERTY()
    bool bUseAsyncCooking = false;
    UPROPERTY()
    bool bEnableComplexCollision = false;
    UPROPERTY()
    bool bDeferCollisionUpdates = false;
    UPROPERTY()
    bool bDisableMeshUVHitResults = false;
    UPROPERTY()
    TObjectPtr<UBodySetup> MeshBodySetup = nullptr;
    UPROPERTY()
    FKAggregateGeom AggGeom;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UBodySetup>> AsyncBodySetupQueue;
};

}
