#pragma once

#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "HierarchicalInstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint8 bUseTranslatedInstanceSpace : 1 = false;
    UPROPERTY()
    FVector TranslatedInstanceSpaceOrigin = FVector::ZeroVector;
    UPROPERTY()
    TArray<int32> SortedInstances;
    UPROPERTY()
    int32 NumBuiltInstances = 0;
    UPROPERTY()
    FBox BuiltInstanceBounds;
    UPROPERTY()
    FBox UnbuiltInstanceBounds;
    UPROPERTY()
    TArray<FBox> UnbuiltInstanceBoundsList;
    UPROPERTY()
    uint8 bEnableDensityScaling : 1 = false;
    UPROPERTY()
    int32 OcclusionLayerNumNodes = 0;
    UPROPERTY()
    FBoxSphereBounds CacheMeshExtendedBounds;
    UPROPERTY()
    int32 InstanceCountToRender = 0;

    void Serialize(FArchive& Ar) override;
};

}
