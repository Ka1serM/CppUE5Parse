#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MRMeshComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;
class UMeshReconstructorBase;
class UMRMeshBodyHolder;

UCLASS()
class UMRMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMRMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> WireframeMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMeshReconstructorBase> MeshReconstructor = nullptr;
    UPROPERTY()
    bool bEnableCollision = false;
    UPROPERTY()
    bool bCreateMeshProxySections = true;
    UPROPERTY()
    bool bUpdateNavMeshOnMeshUpdate = true;
    UPROPERTY()
    bool bNeverCreateCollisionMesh = false;
    UPROPERTY()
    TArray<TObjectPtr<UBodySetup>> BodySetups;
    UPROPERTY()
    TObjectPtr<UBodySetup> CachedBodySetup = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMRMeshBodyHolder>> BodyHolders;
};

}
