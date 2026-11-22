#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperGroupedSpriteActor.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperGroupedSpriteComponent.h"

namespace ue5parse::runtime {

APaperGroupedSpriteActor::APaperGroupedSpriteActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperGroupedSpriteComponent>(FName("RenderComponent"));
    RootComponent = RenderComponent;
}

}
