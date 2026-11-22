#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceActor.h"

namespace ue5parse::runtime {

AMaterialInstanceActor::AMaterialInstanceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
