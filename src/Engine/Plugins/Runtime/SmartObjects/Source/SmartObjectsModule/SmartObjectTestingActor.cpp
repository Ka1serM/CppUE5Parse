#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTestingActor.h"

namespace ue5parse::runtime {

ASmartObjectTestingActor::ASmartObjectTestingActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RenderingComponent = ObjectInitializer.CreateDefaultSubobject<USmartObjectTestRenderingComponent>(FName("RenderingComp"));
    RootComponent = RenderingComponent;
    bCanBeDamaged = false;
}

}
