#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleCollision.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleEvent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Pawn.h"

namespace ue5parse::runtime {

static float GParticleCollisionIgnoreInvisibleTime = .1f;

UParticleModuleCollision::UParticleModuleCollision(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    CollisionCompletionOption = EPCC_Kill;
    bApplyPhysics = false;
    DirScalar = 3.5f;
    VerticalFudgeFactor = 0.1f;
    bDropDetail = true;
    LODDuplicate = false;
    bPawnsDoNotDecrementCount = true;
    bCollideOnlyIfVisible = true;
    MaxCollisionDistance = 1000.0f;
    bIgnoreSourceActor = true;
    bIgnoreTriggerVolumes = true;
    CollisionTypes.Add(ObjectTypeQuery1);
}

uint32 UParticleModuleCollision::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FParticleCollisionPayload);
}

uint32 UParticleModuleCollision::RequiredBytesPerInstance() {
    return sizeof(FParticleCollisionInstancePayload);
}

uint32 UParticleModuleCollision::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) {
    FParticleCollisionInstancePayload* CollisionInstPayload = (FParticleCollisionInstancePayload*)(InstData);
    CollisionInstPayload->CurrentLODBoundsCheckCount = 0;
    return 0;
}

void UParticleModuleCollision::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        PARTICLE_ELEMENT(FParticleCollisionPayload, CollisionPayload);
        CollisionPayload.UsedDampingFactor = FVector3f(DampingFactor.GetValue(Owner->EmitterTime, Owner->Component));
        CollisionPayload.UsedDampingFactorRotation = FVector3f(DampingFactorRotation.GetValue(Owner->EmitterTime, Owner->Component));
        CollisionPayload.UsedCollisions = FMath::RoundToInt(MaxCollisions.GetValue(Owner->EmitterTime, Owner->Component));
        CollisionPayload.Delay = DelayAmount.GetValue(Owner->EmitterTime, Owner->Component);
        if (CollisionPayload.Delay > SpawnTime) {
            Particle.Flags |= STATE_Particle_DelayCollisions;
            Particle.Flags &= ~STATE_Particle_CollisionHasOccurred;
        }
    }
}

void UParticleModuleCollision::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UWorld* World = Owner->Component->GetWorld();
    if (Owner->ActiveParticles == 0 || (bDropDetail && World && World->bDropDetail))
        return;
    AActor* Actor = Owner->Component->GetOwner();
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    const bool bMeshRotationActive = Owner->IsMeshRotationActive();
    const FTransform& OwnerTM = Owner->Component->GetComponentTransform();
    const FVector ParentScale = OwnerTM.GetScale3D();
    FParticleEventInstancePayload* EventPayload = nullptr;
    if (LODLevel->EventGenerator) {
        EventPayload = (FParticleEventInstancePayload*)(Owner->GetModuleInstanceData(LODLevel->EventGenerator));
        if (EventPayload && (EventPayload->bCollisionEventsPresent == false) && (EventPayload->bDeathEventsPresent == false))
            EventPayload = nullptr;
    }
    FParticleCollisionInstancePayload* CollisionInstPayload = (FParticleCollisionInstancePayload*)(Owner->GetModuleInstanceData(this));
    const TArray<FVector>& PlayerLocations = Owner->Component->GetPlayerLocations();
    TArray<float> PlayerLODDistanceFactor = Owner->Component->GetPlayerLODDistanceFactor();
    const int32 PlayerCount = PlayerLocations.Num();
    if (World->IsGameWorld()) {
        bool bIgnoreAllCollision = false;
        if (bCollideOnlyIfVisible && ((World->TimeSeconds - Owner->Component->GetLastRenderTime()) > GParticleCollisionIgnoreInvisibleTime)) {
            bIgnoreAllCollision = true;
        } else if (MaxCollisionDistance < UE_OLD_WORLD_MAX) {
            if (Owner->ActiveParticles > 7) {
                if (CollisionInstPayload->CurrentLODBoundsCheckCount == 0) {
                    FBox BoundingBox;
                    if (Owner->Component->Template && Owner->Component->Template->bUseFixedRelativeBoundingBox)
                        BoundingBox = Owner->Component->Template->FixedRelativeBoundingBox.TransformBy(OwnerTM);
                    else
                        BoundingBox = Owner->Component->Bounds;
                    bIgnoreAllCollision = true;
                    bool bCloseEnough = false;
                    for (int32 PlyrIdx = 0; PlyrIdx < PlayerCount; PlyrIdx++) {
                        float InvDistanceFactor = 1.0f / PlayerLODDistanceFactor[PlyrIdx];
                        FBox CheckBounds = BoundingBox;
                        double BoxExpansionValue = MaxCollisionDistance * InvDistanceFactor;
                        BoxExpansionValue += BoxExpansionValue * 0.075f;
                        CheckBounds = CheckBounds.ExpandBy(BoxExpansionValue);
                        if (CheckBounds.IsInside(PlayerLocations[PlyrIdx])) {
                            bCloseEnough = true;
                            break;
                        }
                    }
                    if (bCloseEnough)
                        bIgnoreAllCollision = false;
                }
                CollisionInstPayload->CurrentLODBoundsCheckCount++;
                if (CollisionInstPayload->CurrentLODBoundsCheckCount > 30)
                    CollisionInstPayload->CurrentLODBoundsCheckCount = 0;
            }
        }
        if (bIgnoreAllCollision) {
            BEGIN_UPDATE_LOOP;
            {
                Particle.Flags |= STATE_Particle_IgnoreCollisions;
            }
            END_UPDATE_LOOP;
            return;
        }
        for (int32 SquareIdx = 0; SquareIdx < PlayerLocations.Num(); SquareIdx++)
            PlayerLODDistanceFactor[SquareIdx] *= PlayerLODDistanceFactor[SquareIdx];
    }
    double SquaredMaxCollisionDistance = FMath::Square<double>(MaxCollisionDistance);
    BEGIN_UPDATE_LOOP;
    {
        if ((Particle.Flags & STATE_Particle_CollisionIgnoreCheck) != 0)
            CONTINUE_UPDATE_LOOP;
        PARTICLE_ELEMENT(FParticleCollisionPayload, CollisionPayload);
        if ((Particle.Flags & STATE_Particle_DelayCollisions) != 0) {
            if (CollisionPayload.Delay > Particle.RelativeTime)
                CONTINUE_UPDATE_LOOP;
            Particle.Flags &= ~STATE_Particle_DelayCollisions;
        }
        FVector Location;
        FVector OldLocation;
        Location = Particle.Location + FVector(Particle.Velocity) * DeltaTime;
        if (LODLevel->RequiredModule->bUseLocalSpace) {
            Location = OwnerTM.TransformPosition(Location);
            OldLocation = OwnerTM.TransformPosition(Particle.OldLocation);
        } else {
            OldLocation = Particle.OldLocation;
        }
        FVector Direction = (Location - OldLocation).GetSafeNormal();
        FVector Size = FVector(Particle.Size) * ParentScale;
        FVector Extent(0.0f);
        UParticleModuleTypeDataMesh* MeshType = Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
        if (MeshType && MeshType->Mesh) {
            Extent = MeshType->Mesh->GetBounds().BoxExtent;
            Extent = MeshType->bCollisionsConsiderPartilceSize ? Extent * Size : Extent;
        }
        FHitResult Hit;
        Hit.Normal.X = 0.0f;
        Hit.Normal.Y = 0.0f;
        Hit.Normal.Z = 0.0f;
        FVector End = Location + Direction * Size / DirScalar;
        if ((World->IsGameWorld()) && (MaxCollisionDistance < UE_OLD_WORLD_MAX)) {
            bool bCloseEnough = false;
            for (int32 CheckIdx = 0; CheckIdx < PlayerCount; CheckIdx++) {
                float CheckValue = (PlayerLocations[CheckIdx] - End).SizeSquared() * PlayerLODDistanceFactor[CheckIdx];
                if (CheckValue < SquaredMaxCollisionDistance) {
                    bCloseEnough = true;
                    break;
                }
            }
            if (bCloseEnough == false) {
                Particle.Flags |= STATE_Particle_IgnoreCollisions;
                CONTINUE_UPDATE_LOOP;
            }
        }
        AActor* IgnoreActor = bIgnoreSourceActor ? Actor : nullptr;
        if (PerformCollisionCheck(Owner, &Particle, Hit, IgnoreActor, End, OldLocation, Extent)) {
            bool bDecrementMaxCount = true;
            bool bIgnoreCollision = false;
            if (Hit.HitObjectHandle.IsValid()) {
                bDecrementMaxCount = !bPawnsDoNotDecrementCount || !Hit.HitObjectHandle.DoesRepresentClass(APawn::StaticClass());
                bIgnoreCollision = bIgnoreTriggerVolumes && Hit.HitObjectHandle.DoesRepresentClass(ATriggerBase::StaticClass());
            }
            if (bIgnoreCollision == false) {
                if (bDecrementMaxCount && (bOnlyVerticalNormalsDecrementCount)) {
                    if ((Hit.Normal.IsNearlyZero() == false) && (FMath::Abs(Hit.Normal.Z) + VerticalFudgeFactor) < 1.0f)
                        bDecrementMaxCount = false;
                }
                if (bDecrementMaxCount)
                    CollisionPayload.UsedCollisions--;
                if (CollisionPayload.UsedCollisions > 0) {
                    if (LODLevel->RequiredModule->bUseLocalSpace) {
                        FVector OldVelocity = OwnerTM.TransformVector(FVector(Particle.Velocity));
                        FVector BaseVelocity = OwnerTM.TransformVector(FVector(Particle.BaseVelocity));
                        BaseVelocity = BaseVelocity.MirrorByVector(Hit.Normal) * FVector(CollisionPayload.UsedDampingFactor);
                        Particle.BaseVelocity = FVector3f(OwnerTM.InverseTransformVector(BaseVelocity));
                        Particle.BaseRotationRate = Particle.BaseRotationRate * CollisionPayload.UsedDampingFactorRotation.X;
                        if (bMeshRotationActive && MeshRotationOffset > 0) {
                            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                            PayloadData->RotationRateBase *= CollisionPayload.UsedDampingFactorRotation;
                        }
                        FVector NewVelocity =
                            Direction.MirrorByVector(Hit.Normal) * (Location - OldLocation).Size() * FVector(CollisionPayload.UsedDampingFactor);
                        Particle.Velocity = FVector3f::ZeroVector;
                        FVector NewLocation = Location + NewVelocity * (1.f - Hit.Time);
                        Particle.Location = OwnerTM.InverseTransformPosition(NewLocation);
                        if (bApplyPhysics) {
                            UPrimitiveComponent* PrimitiveComponent = Hit.GetComponent();
                            if (PrimitiveComponent && PrimitiveComponent->IsAnySimulatingPhysics()) {
                                FVector vImpulse;
                                vImpulse = -(NewVelocity - OldVelocity) * ParticleMass.GetValue(Particle.RelativeTime, Owner->Component);
                                PrimitiveComponent->AddImpulseAtLocation(vImpulse, Hit.Location, Hit.BoneName);
                            }
                        }
                    } else {
                        FVector vOldVelocity(Particle.Velocity);
                        Particle.BaseVelocity = Particle.BaseVelocity.MirrorByVector(FVector3f(Hit.Normal)) * CollisionPayload.UsedDampingFactor;
                        Particle.BaseRotationRate = Particle.BaseRotationRate * CollisionPayload.UsedDampingFactorRotation.X;
                        if (bMeshRotationActive && MeshRotationOffset > 0) {
                            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                            PayloadData->RotationRateBase *= CollisionPayload.UsedDampingFactorRotation;
                        }
                        FVector vNewVelocity =
                            Direction.MirrorByVector(Hit.Normal) * (Location - OldLocation).Size() * FVector(CollisionPayload.UsedDampingFactor);
                        Particle.Velocity = FVector3f::ZeroVector;
                        Particle.Location += vNewVelocity * (1.f - Hit.Time);
                        if (bApplyPhysics) {
                            UPrimitiveComponent* PrimitiveComponent = Hit.GetComponent();
                            if (PrimitiveComponent && PrimitiveComponent->IsAnySimulatingPhysics()) {
                                FVector vImpulse;
                                vImpulse = -(vNewVelocity - vOldVelocity) * ParticleMass.GetValue(Particle.RelativeTime, Owner->Component);
                                PrimitiveComponent->AddImpulseAtLocation(vImpulse, Hit.Location, Hit.BoneName);
                            }
                        }
                    }
                    if (EventPayload && (EventPayload->bCollisionEventsPresent))
                        LODLevel->EventGenerator->HandleParticleCollision(Owner, EventPayload, &CollisionPayload, &Hit, &Particle, Direction);
                } else {
                    if (LODLevel->RequiredModule->bUseLocalSpace)
                        Size = OwnerTM.TransformVector(Size);
                    Particle.Location = Hit.Location;
                    if (LODLevel->RequiredModule->bUseLocalSpace)
                        Particle.Location = OwnerTM.InverseTransformPosition(Particle.Location);
                    switch (CollisionCompletionOption) {
                    case EPCC_Kill: {
                        if (EventPayload && (EventPayload->bDeathEventsPresent))
                            LODLevel->EventGenerator->HandleParticleKilled(Owner, EventPayload, &Particle);
                        KILL_CURRENT_PARTICLE;
                    } break;
                    case EPCC_Freeze: {
                        Particle.Flags |= STATE_Particle_Freeze;
                    } break;
                    case EPCC_HaltCollisions: {
                        Particle.Flags |= STATE_Particle_IgnoreCollisions;
                    } break;
                    case EPCC_FreezeTranslation: {
                        Particle.Flags |= STATE_Particle_FreezeTranslation;
                    } break;
                    case EPCC_FreezeRotation: {
                        Particle.Flags |= STATE_Particle_FreezeRotation;
                    } break;
                    case EPCC_FreezeMovement: {
                        Particle.Flags |= STATE_Particle_FreezeRotation;
                        Particle.Flags |= STATE_Particle_FreezeTranslation;
                    } break;
                    }
                    if (EventPayload && (EventPayload->bCollisionEventsPresent))
                        LODLevel->EventGenerator->HandleParticleCollision(Owner, EventPayload, &CollisionPayload, &Hit, &Particle, Direction);
                }
                Particle.Flags |= STATE_Particle_CollisionHasOccurred;
            }
        }
    }
    END_UPDATE_LOOP;
}

bool UParticleModuleCollision::PerformCollisionCheck(FParticleEmitterInstance* Owner, FBaseParticle* InParticle, FHitResult& Hit, AActor* SourceActor,
    const FVector& End, const FVector& Start, const FVector& Extent) {
    return Owner->Component->ParticleLineCheck(Hit, SourceActor, End, Start, Extent, CollisionTypes);
}

}
