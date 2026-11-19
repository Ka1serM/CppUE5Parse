#include "Engine/Source/Runtime/Engine/Particles/Emitter.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

AEmitter::AEmitter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ParticleSystemComponent = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(FName("ParticleSystemComponent0"));
    ParticleSystemComponent->SecondsBeforeInactive = 1;
    RootComponent = ParticleSystemComponent;
}

}
