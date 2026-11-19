#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavCollisionBase.h"
#include "Engine/Source/Runtime/NavigationSystem/NavAreas/NavArea.h"

#include "NavCollision.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNavCollisionCylinder {
    GENERATED_BODY()
    UPROPERTY()
    FVector Offset = FVector::ZeroVector;
    UPROPERTY()
    float Radius = 0.0f;
    UPROPERTY()
    float Height = 0.0f;
};

USTRUCT()
struct FNavCollisionBox {
    GENERATED_BODY()
    UPROPERTY()
    FVector Offset = FVector::ZeroVector;
    UPROPERTY()
    FVector Extent = FVector::ZeroVector;
};

UCLASS()
class UNavCollision : public UNavCollisionBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FNavCollisionCylinder> CylinderCollision;
    UPROPERTY()
    TArray<FNavCollisionBox> BoxCollision;
    UPROPERTY()
    TSubclassOf<UNavArea> AreaClass;
    UPROPERTY()
    bool bGatherConvexGeometry = true;
    UPROPERTY()
    bool bCreateOnClient = true;

    FFormatContainer CookedFormatData;

    void Serialize(FArchive& Ar) override;
};

}
