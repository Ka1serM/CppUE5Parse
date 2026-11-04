#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleVelocity.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

UParticleModuleVelocity::UParticleModuleVelocity(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleVelocity::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
        FVector FromOrigin = (Particle.Location - Owner->EmitterToSimulation.GetOrigin()).GetSafeNormal();
        FVector OwnerScale(1.0f);
        if ((bApplyOwnerScale) && Owner->Component)
            OwnerScale = Owner->Component->GetComponentTransform().GetScale3D();
        UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
        if (LODLevel->RequiredModule->bUseLocalSpace)
            if (bInWorldSpace)
                Vel = Owner->SimulationToWorld.InverseTransformVector(Vel);
            else
                Vel = Owner->EmitterToSimulation.TransformVector(Vel);
        else if (bInWorldSpace == false)
            Vel = Owner->EmitterToSimulation.TransformVector(Vel);
        Vel *= OwnerScale;
        Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream) * OwnerScale;
        Particle.Velocity += FVector3f(Vel);
        Particle.BaseVelocity += FVector3f(Vel);
    }
}

UParticleModuleVelocity_Seeded::UParticleModuleVelocity_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleVelocity_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleVelocityInheritParent::UParticleModuleVelocityInheritParent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModuleVelocityInheritParent::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector Vel(0.0f);
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (LODLevel->RequiredModule->bUseLocalSpace)
        Vel = Owner->Component->GetComponentTransform().InverseTransformVector(Owner->Component->PartSysVelocity);
    else
        Vel = Owner->Component->PartSysVelocity;
    FVector vScale = Scale.GetValue(Owner->EmitterTime, Owner->Component);
    Vel *= vScale;
    Particle.Velocity += FVector3f(Vel);
    Particle.BaseVelocity += FVector3f(Vel);
}

UParticleModuleVelocityOverLifetime::UParticleModuleVelocityOverLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    Absolute = false;
}

void UParticleModuleVelocityOverLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    if (Absolute) {
        SPAWN_INIT;
        FVector OwnerScale(1.0f);
        if ((bApplyOwnerScale) && Owner && Owner->Component)
            OwnerScale = Owner->Component->GetComponentTransform().GetScale3D();
        FVector Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
        Particle.Velocity = FVector3f(Vel);
        Particle.BaseVelocity = FVector3f(Vel);
    }
}

void UParticleModuleVelocityOverLifetime::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    FVector OwnerScale(1.0f);
    const FTransform& OwnerTM = Owner->Component->GetComponentTransform();
    if (bApplyOwnerScale)
        OwnerScale = OwnerTM.GetScale3D();
    if (Absolute) {
        if (LODLevel->RequiredModule->bUseLocalSpace == false) {
            if (bInWorldSpace == false) {
                FVector Vel;
                const FMatrix LocalToWorld = OwnerTM.ToMatrixNoScale();
                BEGIN_UPDATE_LOOP;
                {
                    Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
                    Particle.Velocity = FVector4f(LocalToWorld.TransformVector(Vel) * OwnerScale);
                }
                END_UPDATE_LOOP;
            } else {
                BEGIN_UPDATE_LOOP;
                {
                    Particle.Velocity = FVector3f(VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale);
                }
                END_UPDATE_LOOP;
            }
        } else if (bInWorldSpace == false) {
            BEGIN_UPDATE_LOOP;
            {
                Particle.Velocity = FVector3f(VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale);
            }
            END_UPDATE_LOOP;
        } else {
            FVector Vel;
            const FMatrix LocalToWorld = OwnerTM.ToMatrixNoScale();
            const FMatrix InvMat = LocalToWorld.InverseFast();
            BEGIN_UPDATE_LOOP;
            {
                Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
                Particle.Velocity = FVector4f(InvMat.TransformVector(Vel) * OwnerScale);
            }
            END_UPDATE_LOOP;
        }
    } else if (LODLevel->RequiredModule->bUseLocalSpace == false) {
        FVector Vel;
        if (bInWorldSpace == false) {
            const FMatrix LocalToWorld = OwnerTM.ToMatrixNoScale();
            BEGIN_UPDATE_LOOP;
            {
                Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
                Particle.Velocity *= FVector4f(LocalToWorld.TransformVector(Vel) * OwnerScale);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                Particle.Velocity *= FVector3f(VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale);
            }
            END_UPDATE_LOOP;
        }
    } else if (bInWorldSpace == false) {
        BEGIN_UPDATE_LOOP;
        {
            Particle.Velocity *= FVector3f(VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale);
        }
        END_UPDATE_LOOP;
    } else {
        FVector Vel;
        const FMatrix LocalToWorld = OwnerTM.ToMatrixNoScale();
        const FMatrix InvMat = LocalToWorld.InverseFast();
        BEGIN_UPDATE_LOOP;
        {
            Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
            Particle.Velocity *= FVector4f(InvMat.TransformVector(Vel) * OwnerScale);
        }
        END_UPDATE_LOOP;
    }
}

UParticleModuleVelocityCone::UParticleModuleVelocityCone(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupported3DDrawMode = true;
    Direction = FVector(0.0f, 0.0f, 1.0f);
}

void UParticleModuleVelocityCone::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleVelocityCone::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    static const float TwoPI = 2.0f * UE_PI;
    static const float ToRads = UE_PI / 180.0f;
    static const int32 UUPerRad = 10430;
    static const FVector DefaultDirection(0.0f, 0.0f, 1.0f);
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    FVector OwnerScale(1.0f);
    if ((bApplyOwnerScale) && Owner->Component)
        OwnerScale = Owner->Component->GetComponentTransform().GetScale3D();
    SPAWN_INIT {
        const float SpawnAngle = Angle.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        const float SpawnVelocity = Velocity.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        const float LatheAngle = InRandomStream->FRand() * TwoPI;
        const FRotator DefaultDirectionRotater((int32)(SpawnAngle * ToRads * UUPerRad), (int32)(LatheAngle * UUPerRad), 0);
        const FRotationMatrix DefaultDirectionRotation(DefaultDirectionRotater);
        const FVector DefaultSpawnDirection = DefaultDirectionRotation.TransformVector(DefaultDirection);
        FVector ForwardDirection = DefaultDirection;
        if (Direction != FVector::ZeroVector)
            ForwardDirection = Direction.GetSafeNormal();
        FVector UpDirection(0.0f, 0.0f, 1.0f);
        FVector RightDirection(1.0f, 0.0f, 0.0f);
        if ((ForwardDirection != UpDirection) && (-ForwardDirection != UpDirection)) {
            RightDirection = UpDirection ^ ForwardDirection;
            UpDirection = ForwardDirection ^ RightDirection;
        } else {
            UpDirection = ForwardDirection ^ RightDirection;
            RightDirection = UpDirection ^ ForwardDirection;
        }
        FMatrix DirectionRotation;
        DirectionRotation.SetIdentity();
        DirectionRotation.SetAxis(0, RightDirection.GetSafeNormal());
        DirectionRotation.SetAxis(1, UpDirection.GetSafeNormal());
        DirectionRotation.SetAxis(2, ForwardDirection);
        FVector SpawnDirection = DirectionRotation.TransformVector(DefaultSpawnDirection);
        if (!LODLevel->RequiredModule->bUseLocalSpace && !bInWorldSpace)
            SpawnDirection = Owner->Component->GetComponentTransform().TransformVector(SpawnDirection);
        else if (LODLevel->RequiredModule->bUseLocalSpace && bInWorldSpace)
            SpawnDirection = Owner->Component->GetComponentTransform().InverseTransformVector(SpawnDirection);
        const FVector3f FinalVelocity(SpawnDirection * SpawnVelocity * OwnerScale);
        Particle.Velocity += FinalVelocity;
        Particle.BaseVelocity += FinalVelocity;
    }
}

}
