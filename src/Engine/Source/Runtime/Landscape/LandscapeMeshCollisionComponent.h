#pragma once

#include "Engine/Source/Runtime/Landscape/LandscapeHeightfieldCollisionComponent.h"

#include "LandscapeMeshCollisionComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULandscapeMeshCollisionComponent : public ULandscapeHeightfieldCollisionComponent {
    GENERATED_BODY()
public:
    explicit ULandscapeMeshCollisionComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FGuid MeshGuid;

    void Serialize(FArchive& Ar) override;
};

}
