#include "Engine/Source/Runtime/Engine/Engine/PlanarReflection.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"
#include "Engine/Source/Runtime/Engine/Components/PlanarReflectionComponent.h"

namespace ue5parse::runtime {

APlanarReflection::APlanarReflection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PlanarReflectionComponent = ObjectInitializer.CreateDefaultSubobject<UPlanarReflectionComponent>(FName("NewPlanarReflectionComponent"));
    RootComponent = PlanarReflectionComponent;
    auto* DrawInfluenceBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("DrawBox0"));
    DrawInfluenceBox->SetupAttachment(PlanarReflectionComponent);
    DrawInfluenceBox->bUseEditorCompositing = true;
    DrawInfluenceBox->BodyInstance.CollisionProfileName = FName("NoCollision");
    PlanarReflectionComponent->PreviewBox = DrawInfluenceBox;
}

}
