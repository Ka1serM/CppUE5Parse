#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Foliage/FoliageInstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Foliage/FoliageType.h"

#include "FoliageType_Actor.generated.h"

namespace ue5parse::runtime {

class AActor;

UCLASS()
class UFoliageType_Actor : public UFoliageType {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<AActor> ActorClass;
    UPROPERTY()
    bool bShouldAttachToBaseComponent = true;
    UPROPERTY()
    bool bStaticMeshOnly = false;
    UPROPERTY()
    TSubclassOf<UFoliageInstancedStaticMeshComponent> StaticMeshOnlyComponentClass;
};

}
