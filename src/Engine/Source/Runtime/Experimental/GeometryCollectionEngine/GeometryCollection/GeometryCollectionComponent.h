#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "GeometryCollectionComponent.generated.h"

namespace ue5parse::runtime {

class UPhysicalMaterial;
class UObject;

UCLASS()
class UGeometryCollectionComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UGeometryCollectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UObject> RestCollection = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> DynamicCollection = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> PhysicalMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysicalMaterialOverride = nullptr;
};

}
