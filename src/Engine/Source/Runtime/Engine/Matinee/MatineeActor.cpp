#include "Engine/Source/Runtime/Engine/Matinee/MatineeActor.h"

namespace ue5parse::runtime {

AMatineeActor::AMatineeActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
