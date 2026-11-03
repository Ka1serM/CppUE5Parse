#include "ue5parse/runtime/Engine/Particles/ParticleModuleSourceMovement.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"
#include "ue5parse/runtime/Engine/Animation/SkeletalMeshActor.h"

// UE's s_ParticleModules.cpp (UE 5.3).

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
