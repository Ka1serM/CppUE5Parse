#include "Engine/Plugins/Runtime/CableComponent/Source/CableComponent/CableActor.h"

#include "Engine/Plugins/Runtime/CableComponent/Source/CableComponent/CableComponent.h"

namespace ue5parse::runtime {

ACableActor::ACableActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CableComponent = ObjectInitializer.CreateDefaultSubobject<UCableComponent>(FName("CableComponent0"));
    RootComponent = CableComponent;
}

}
