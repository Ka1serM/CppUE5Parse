#include "Engine/Source/Runtime/NavigationSystem/NavSystemConfigOverride.h"

namespace ue5parse::runtime {

ANavSystemConfigOverride::ANavSystemConfigOverride(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    RootComponent->Mobility = EComponentMobility::Static;
}

}
