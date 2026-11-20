#include "Engine/Source/Runtime/Engine/Engine/TextRenderActor.h"

#include "Engine/Source/Runtime/Engine/Components/TextRenderComponent.h"

namespace ue5parse::runtime {

ATextRenderActor::ATextRenderActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    TextRender = ObjectInitializer.CreateDefaultSubobject<UTextRenderComponent>(FName("NewTextRenderComponent"));
    RootComponent = TextRender;
}

}
