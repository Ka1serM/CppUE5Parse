#include "Engine/Source/Runtime/Engine/Engine/Note.h"

namespace ue5parse::runtime {

ANote::ANote(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    RootComponent->Mobility = EComponentMobility::Static;
}

}
