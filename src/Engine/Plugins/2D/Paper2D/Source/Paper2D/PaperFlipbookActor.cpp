#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperFlipbookActor.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperFlipbookComponent.h"

namespace ue5parse::runtime {

APaperFlipbookActor::APaperFlipbookActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperFlipbookComponent>(FName("RenderComponent"));
    RootComponent = RenderComponent;
}

}
