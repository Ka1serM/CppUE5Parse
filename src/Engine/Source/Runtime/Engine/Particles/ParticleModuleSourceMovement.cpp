#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSourceMovement.h"

#include "Engine/Source/Runtime/Engine/Animation/SkeletalMeshActor.h"
#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleSourceMovement::UParticleModuleSourceMovement(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bFinalUpdateModule = true;
}

void UParticleModuleSourceMovement::FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    Super::FinalUpdate(Owner, Offset, DeltaTime);
    if (Owner && Owner->Component) {
        FVector FrameDiff = Owner->Component->GetComponentLocation() - Owner->Component->OldPosition;
        BEGIN_UPDATE_LOOP;
        {
            if (Particle.RelativeTime > (2.0f * DeltaTime * Particle.OneOverMaxLifetime)) {
                FVector Scale = SourceMovementScale.GetValue(Particle.RelativeTime, Owner->Component);
                Particle.Location += (Scale * FrameDiff);
            }
        }
        END_UPDATE_LOOP;
    }
}

}
