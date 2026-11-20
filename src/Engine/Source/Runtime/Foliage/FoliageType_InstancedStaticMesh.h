#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Foliage/FoliageInstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Foliage/FoliageType.h"

#include "FoliageType_InstancedStaticMesh.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UStaticMesh;

UCLASS()
class UFoliageType_InstancedStaticMesh : public UFoliageType {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UStaticMesh> Mesh = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> NaniteOverrideMaterials;
    UPROPERTY()
    TSubclassOf<UFoliageInstancedStaticMeshComponent> ComponentClass;
};

}
