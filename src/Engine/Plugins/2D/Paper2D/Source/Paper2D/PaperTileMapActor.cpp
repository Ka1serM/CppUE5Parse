#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTileMapActor.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTileMapComponent.h"
#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTileMapRenderComponent.h"

namespace ue5parse::runtime {

APaperTileMapActor::APaperTileMapActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_7)
        RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperTileMapRenderComponent>(FName("RenderComponent"));
    else
        RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperTileMapComponent>(FName("RenderComponent"));
    RootComponent = RenderComponent;
}

}
