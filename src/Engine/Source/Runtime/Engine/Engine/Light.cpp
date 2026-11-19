#include "Engine/Source/Runtime/Engine/Engine/Light.h"

namespace ue5parse::runtime {

ALight::ALight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LightComponent = ObjectInitializer.CreateDefaultSubobject<ULightComponent>(FName("LightComponent0"));
    bCollideWhenPlacing = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}

}
