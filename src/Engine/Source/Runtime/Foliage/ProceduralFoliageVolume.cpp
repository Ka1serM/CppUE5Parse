#include "Engine/Source/Runtime/Foliage/ProceduralFoliageVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"
#include "Engine/Source/Runtime/Foliage/ProceduralFoliageComponent.h"

namespace ue5parse::runtime {

AProceduralFoliageVolume::AProceduralFoliageVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ProceduralComponent = ObjectInitializer.CreateDefaultSubobject<UProceduralFoliageComponent>(FName("ProceduralFoliageComponent"));
    ProceduralComponent->SpawningVolume = this;
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("Custom");
    GetBrushComponent()->bGenerateOverlapEvents = false;
}

}
