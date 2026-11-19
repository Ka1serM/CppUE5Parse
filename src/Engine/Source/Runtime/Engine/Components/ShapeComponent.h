#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"

#include "ShapeComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UNavAreaBase;

UCLASS(Abstract)
class UShapeComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FColor ShapeColor = FColor(223, 149, 157, 255);
    UPROPERTY()
    TObjectPtr<UMaterialInterface> ShapeMaterial = nullptr;
    UPROPERTY()
    uint8 bDrawOnlyIfSelected : 1 = false;
    UPROPERTY()
    uint8 bShouldCollideWhenPlacing : 1 = false;
    UPROPERTY()
    uint8 bDynamicObstacle : 1 = false;
    UPROPERTY()
    TSubclassOf<UNavAreaBase> AreaClass;
    UPROPERTY()
    TSubclassOf<UNavAreaBase> AreaClassOverride;
    UPROPERTY()
    uint8 bUseSystemDefaultObstacleAreaClass : 1 = true;
    UPROPERTY()
    float LineThickness = 0.0f;

    explicit UShapeComponent(const FObjectInitializer& ObjectInitializer);
};

}
