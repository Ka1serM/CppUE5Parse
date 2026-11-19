#include "Engine/Source/Runtime/Engine/Engine/LightmassPortal.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"
#include "Engine/Source/Runtime/Engine/Components/LightmassPortalComponent.h"

namespace ue5parse::runtime {

ALightmassPortal::ALightmassPortal(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PortalComponent = ObjectInitializer.CreateDefaultSubobject<ULightmassPortalComponent>(FName("PortalComponent"));
    PortalComponent->SetRelativeScale3D_Direct(FVector(10, 100, 100));
    RootComponent = PortalComponent;
    auto* DrawInfluenceBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("DrawBox0"));
    DrawInfluenceBox->SetupAttachment(PortalComponent);
    DrawInfluenceBox->bUseEditorCompositing = true;
    DrawInfluenceBox->BodyInstance.CollisionProfileName = FName("NoCollision");
    DrawInfluenceBox->BoxExtent = FVector(1, 1, 1);
    PortalComponent->PreviewBox = DrawInfluenceBox;
}

}
