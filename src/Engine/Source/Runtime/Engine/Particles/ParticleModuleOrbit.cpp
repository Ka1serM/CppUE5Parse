#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleOrbit.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleOrbit::UParticleModuleOrbit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleOrbit::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    SPAWN_INIT;
    {
        PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
        if (OffsetOptions.bProcessDuringSpawn)
        {
            FVector LocalOffset;
            if (OffsetOptions.bUseEmitterTime == false)
            {
                LocalOffset = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalOffset = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.BaseOffset += FVector3f(LocalOffset);
            OrbitPayload.PreviousOffset = OrbitPayload.Offset;
            OrbitPayload.Offset += FVector3f(LocalOffset);
        }
        if (RotationOptions.bProcessDuringSpawn)
        {
            FVector LocalRotation;
            if (RotationOptions.bUseEmitterTime == false)
            {
                LocalRotation = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalRotation = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.Rotation += FVector3f(LocalRotation);
        }
        if (RotationRateOptions.bProcessDuringSpawn)
        {
            FVector LocalRotationRate;
            if (RotationRateOptions.bUseEmitterTime == false)
            {
                LocalRotationRate = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalRotationRate = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.BaseRotationRate += FVector3f(LocalRotationRate);
            OrbitPayload.RotationRate += FVector3f(LocalRotationRate);
        }
    }
}
void UParticleModuleOrbit::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    BEGIN_UPDATE_LOOP;
    {
        PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
        if (OffsetOptions.bProcessDuringUpdate)
        {
            FVector LocalOffset;
            if (OffsetOptions.bUseEmitterTime == false)
            {
                LocalOffset = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalOffset = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.PreviousOffset = OrbitPayload.Offset;
            OrbitPayload.Offset += FVector3f(LocalOffset);
        }
        if (RotationOptions.bProcessDuringUpdate)
        {
            FVector LocalRotation;
            if (RotationOptions.bUseEmitterTime == false)
            {
                LocalRotation = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalRotation = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.Rotation += FVector3f(LocalRotation);
        }
        if (RotationRateOptions.bProcessDuringUpdate)
        {
            FVector LocalRotationRate;
            if (RotationRateOptions.bUseEmitterTime == false)
            {
                LocalRotationRate = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
            }
            else
            {
                LocalRotationRate = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
            }
            OrbitPayload.RotationRate += FVector3f(LocalRotationRate);
        }
    }
    END_UPDATE_LOOP;
}
uint32 UParticleModuleOrbit::RequiredBytes(UParticleModuleTypeDataBase* TypeData) { return sizeof(FOrbitChainModuleInstancePayload); }

}
