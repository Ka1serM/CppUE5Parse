#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleCamera.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleCameraOffset::UParticleModuleCameraOffset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bSpawnTimeOnly = false;
    UpdateMethod = EPCOUM_DirectSet;
}

void UParticleModuleCameraOffset::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    float ScaleFactor = 1.0f;
    UParticleLODLevel* LODLevel = Owner ? Owner->SpriteTemplate->GetCurrentLODLevel(Owner) : nullptr;
    if (LODLevel && LODLevel->RequiredModule->bUseLocalSpace == false) {
        if (Owner != nullptr && Owner->Component != nullptr)
            ScaleFactor = Owner->Component->GetComponentTransform().GetMaximumAxisScale();
    }
    SPAWN_INIT;
    {
        CurrentOffset = Owner ? ((Owner->CameraPayloadOffset != 0) ? Owner->CameraPayloadOffset : Offset) : Offset;
        PARTICLE_ELEMENT(FCameraOffsetParticlePayload, CameraPayload);
        float CameraOffsetValue = CameraOffset.GetValue(Particle.RelativeTime, Owner->Component) * ScaleFactor;
        if (UpdateMethod == EPCOUM_DirectSet) {
            CameraPayload.BaseOffset = CameraOffsetValue;
            CameraPayload.Offset = CameraOffsetValue;
        } else if (UpdateMethod == EPCOUM_Additive) {
            CameraPayload.Offset += CameraOffsetValue;
        } else {
            CameraPayload.Offset *= CameraOffsetValue;
        }
    }
}

void UParticleModuleCameraOffset::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if (bSpawnTimeOnly == false) {
        BEGIN_UPDATE_LOOP;
        {
            CurrentOffset = Owner ? ((Owner->CameraPayloadOffset != 0) ? Owner->CameraPayloadOffset : Offset) : Offset;
            PARTICLE_ELEMENT(FCameraOffsetParticlePayload, CameraPayload);
            float CameraOffsetValue = CameraOffset.GetValue(Particle.RelativeTime, Owner->Component);
            if (UpdateMethod == EPCOUM_Additive)
                CameraPayload.Offset += CameraOffsetValue;
            else if (UpdateMethod == EPCOUM_Scalar)
                CameraPayload.Offset *= CameraOffsetValue;
            else
                CameraPayload.Offset = CameraOffsetValue;
        }
        END_UPDATE_LOOP;
    }
}

uint32 UParticleModuleCameraOffset::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FCameraOffsetParticlePayload);
}

}
