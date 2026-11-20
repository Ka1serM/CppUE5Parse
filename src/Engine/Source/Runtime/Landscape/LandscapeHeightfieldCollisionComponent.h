#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "LandscapeHeightfieldCollisionComponent.generated.h"

namespace ue5parse::runtime {

class ULandscapeComponent;
class ULandscapeLayerInfoObject;
class UPhysicalMaterial;

UCLASS()
class ULandscapeHeightfieldCollisionComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit ULandscapeHeightfieldCollisionComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FName> ComponentLayers_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeLayerInfoObject>> ComponentLayerInfos;
    UPROPERTY()
    int32 SectionBaseX = 0;
    UPROPERTY()
    int32 SectionBaseY = 0;
    UPROPERTY()
    int32 CollisionSizeQuads = 0;
    UPROPERTY()
    float CollisionScale = 0.0f;
    UPROPERTY()
    int32 SimpleCollisionSizeQuads = 0;
    UPROPERTY()
    TArray<uint8> CollisionQuadFlags;
    UPROPERTY()
    FGuid HeightfieldGuid;
    UPROPERTY()
    FBox CachedLocalBox;
    UPROPERTY()
    FBoxSphereBounds CachedBoxSphereBounds_DEPRECATED;
    UPROPERTY()
    TLazyObjectPtr<ULandscapeComponent> RenderComponent;
    UPROPERTY()
    TObjectPtr<ULandscapeComponent> RenderComponentRef = nullptr;
    UPROPERTY()
    uint32 CollisionHash = 0;
    UPROPERTY()
    TArray<TObjectPtr<UPhysicalMaterial>> PhysicalMaterialRenderObjects;
    UPROPERTY()
    TArray<TObjectPtr<UPhysicalMaterial>> CookedPhysicalMaterials;

    TArray<uint8> CookedCollisionData;

    void Serialize(FArchive& Ar) override;
};

}
