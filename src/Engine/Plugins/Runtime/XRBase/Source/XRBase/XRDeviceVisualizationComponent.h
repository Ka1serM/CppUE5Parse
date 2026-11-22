#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "XRDeviceVisualizationComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UStaticMesh;

UCLASS()
class UXRDeviceVisualizationComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UXRDeviceVisualizationComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bIsVisualizationActive = false;
    UPROPERTY()
    FName DisplayModelSource;
    UPROPERTY()
    TObjectPtr<UStaticMesh> CustomDisplayMesh = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> DisplayMeshMaterialOverrides;
};

}
