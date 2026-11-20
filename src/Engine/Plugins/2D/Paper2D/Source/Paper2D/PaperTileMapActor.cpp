#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTileMapActor.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTileMapComponent.h"

namespace ue5parse::runtime {

APaperTileMapActor::APaperTileMapActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperTileMapComponent>(FName("RenderComponent"));
    RootComponent = RenderComponent;
}

}
