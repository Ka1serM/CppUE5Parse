#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperSpriteActor.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperSpriteComponent.h"

namespace ue5parse::runtime {

APaperSpriteActor::APaperSpriteActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperSpriteComponent>(FName("RenderComponent"));
    RenderComponent->Mobility = EComponentMobility::Static;
    RootComponent = RenderComponent;
}

}
