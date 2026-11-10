#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleAcceleration.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleAccelerationConstant::UParticleModuleAccelerationConstant(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleAccelerationConstant::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (bAlwaysInWorldSpace && LODLevel->RequiredModule->bUseLocalSpace) {
        FVector3f LocalAcceleration(Owner->Component->GetComponentTransform().InverseTransformVector(Acceleration));
        Particle.Velocity += LocalAcceleration * SpawnTime;
        Particle.BaseVelocity += LocalAcceleration * SpawnTime;
    } else {
        FVector3f LocalAcceleration(Acceleration);
        if (LODLevel->RequiredModule->bUseLocalSpace)
            LocalAcceleration = FVector4f(Owner->EmitterToSimulation.TransformVector(FVector(LocalAcceleration)));
        Particle.Velocity += LocalAcceleration * SpawnTime;
        Particle.BaseVelocity += LocalAcceleration * SpawnTime;
    }
}

void UParticleModuleAccelerationConstant::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if ((Owner == nullptr) || (Owner->ActiveParticles <= 0) || (Owner->ParticleData == nullptr) || (Owner->ParticleIndices == nullptr))
        return;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (bAlwaysInWorldSpace && LODLevel->RequiredModule->bUseLocalSpace) {
        FTransform Mat = Owner->Component->GetComponentTransform();
        FVector3f LocalAcceleration(Mat.InverseTransformVector(Acceleration));
        BEGIN_UPDATE_LOOP;
        {
            Particle.Velocity += LocalAcceleration * DeltaTime;
            Particle.BaseVelocity += LocalAcceleration * DeltaTime;
        }
        END_UPDATE_LOOP;
    } else {
        FVector3f LocalAcceleration(Acceleration);
        if (LODLevel->RequiredModule->bUseLocalSpace)
            LocalAcceleration = FVector4f(Owner->EmitterToSimulation.TransformVector(FVector(LocalAcceleration)));
        BEGIN_UPDATE_LOOP;
        {
            Particle.Velocity += LocalAcceleration * DeltaTime;
            Particle.BaseVelocity += LocalAcceleration * DeltaTime;
        }
        END_UPDATE_LOOP;
    }
}

UParticleModuleAccelerationDrag::UParticleModuleAccelerationDrag(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = false;
    bUpdateModule = true;
}

void UParticleModuleAccelerationDrag::PostLoad() {
    Super::PostLoad();
}

void UParticleModuleAccelerationDrag::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    BEGIN_UPDATE_LOOP;
    {
        FVector3f Drag = Particle.Velocity * -DragCoefficientRaw.GetValue(Particle.RelativeTime, Owner->Component);
        Particle.Velocity += Drag * DeltaTime;
        Particle.BaseVelocity += Drag * DeltaTime;
    }
    END_UPDATE_LOOP;
}

void UParticleModuleAccelerationDragScaleOverLife::PostLoad() {
    Super::PostLoad();
}

UParticleModuleAcceleration::UParticleModuleAcceleration(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleAcceleration::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    PARTICLE_ELEMENT(FVector3f, UsedAcceleration);
    UsedAcceleration = FVector3f(Acceleration.GetValue(Owner->EmitterTime, Owner->Component));
    if ((bApplyOwnerScale) && Owner && Owner->Component) {
        FVector3f Scale(Owner->Component->GetComponentTransform().GetScale3D());
        UsedAcceleration *= Scale;
    }
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (bAlwaysInWorldSpace && LODLevel->RequiredModule->bUseLocalSpace) {
        FVector3f TempUsedAcceleration = FVector3f(Owner->Component->GetComponentTransform().InverseTransformVector(FVector(UsedAcceleration)));
        Particle.Velocity += TempUsedAcceleration * SpawnTime;
        Particle.BaseVelocity += TempUsedAcceleration * SpawnTime;
    } else {
        if (LODLevel->RequiredModule->bUseLocalSpace)
            UsedAcceleration = FVector4f(Owner->EmitterToSimulation.TransformVector(FVector(UsedAcceleration)));
        Particle.Velocity += UsedAcceleration * SpawnTime;
        Particle.BaseVelocity += UsedAcceleration * SpawnTime;
    }
}

void UParticleModuleAcceleration::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if ((Owner == nullptr) || (Owner->ActiveParticles <= 0) || (Owner->ParticleData == nullptr) || (Owner->ParticleIndices == nullptr))
        return;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (bAlwaysInWorldSpace && LODLevel->RequiredModule->bUseLocalSpace) {
        FTransform Mat = Owner->Component->GetComponentTransform();
        BEGIN_UPDATE_LOOP;
        {
            FVector3f& UsedAcceleration = *((FVector3f*)(ParticleBase + CurrentOffset));
            FVector3f TransformedUsedAcceleration = FVector3f(Mat.InverseTransformVector((FVector)UsedAcceleration));
            Particle.Velocity += TransformedUsedAcceleration * DeltaTime;
            Particle.BaseVelocity += TransformedUsedAcceleration * DeltaTime;
        }
        END_UPDATE_LOOP;
    } else {
        BEGIN_UPDATE_LOOP;
        {
            FVector3f& UsedAcceleration = *((FVector3f*)(ParticleBase + CurrentOffset));
            Particle.Velocity += UsedAcceleration * DeltaTime;
            Particle.BaseVelocity += UsedAcceleration * DeltaTime;
        }
        END_UPDATE_LOOP;
    }
}

uint32 UParticleModuleAcceleration::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FVector3f);
}

UParticleModuleAccelerationOverLifetime::UParticleModuleAccelerationOverLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = false;
    bUpdateModule = true;
}

void UParticleModuleAccelerationOverLifetime::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (bAlwaysInWorldSpace && LODLevel->RequiredModule->bUseLocalSpace) {
        FTransform Mat = Owner->Component->GetComponentTransform();
        BEGIN_UPDATE_LOOP;
        FVector Accel = AccelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
        Accel = Mat.InverseTransformVector(Accel);
        Particle.Velocity += FVector3f(Accel) * DeltaTime;
        Particle.BaseVelocity += FVector3f(Accel) * DeltaTime;
        END_UPDATE_LOOP;
    } else {
        BEGIN_UPDATE_LOOP;
        FVector Accel = AccelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
        Particle.Velocity += FVector3f(Accel) * DeltaTime;
        Particle.BaseVelocity += FVector3f(Accel) * DeltaTime;
        END_UPDATE_LOOP;
    }
}

}
