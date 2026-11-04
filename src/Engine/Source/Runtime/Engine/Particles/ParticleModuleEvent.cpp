#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleEvent.h"

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleEventGenerator::UParticleModuleEventGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

uint32 UParticleModuleEventGenerator::RequiredBytesPerInstance() { return sizeof(FParticleEventInstancePayload); }

uint32 UParticleModuleEventGenerator::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) {
    FParticleEventInstancePayload* Payload = (FParticleEventInstancePayload*)InstData;
    if (!Payload)
        return 0xffffffff;
    for (const FParticleEvent_GenerateInfo& Event : Events) {
        switch (Event.Type) {
        case EPET_Spawn:
            Payload->bSpawnEventsPresent = true;
            break;
        case EPET_Death:
            Payload->bDeathEventsPresent = true;
            break;
        case EPET_Collision:
            Payload->bCollisionEventsPresent = true;
            break;
        case EPET_Burst:
            Payload->bBurstEventsPresent = true;
            break;
        default:
            break;
        }
    }
    return 0;
}

bool UParticleModuleEventGenerator::HandleParticleSpawned(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload,
    FBaseParticle* NewParticle) {
    EventPayload->SpawnTrackingCount++;
    bool bProcessed = false;
    for (FParticleEvent_GenerateInfo& EventGenInfo : Events) {
        if (EventGenInfo.Type != EPET_Spawn)
            continue;
        if (EventGenInfo.Frequency == 0 || (EventPayload->SpawnTrackingCount % EventGenInfo.Frequency) == 0) {
            const FVector ParticleLocation = EventGenInfo.bUseOrbitOffset ? Owner->GetParticleLocationWithOrbitOffset(NewParticle) : FVector(NewParticle->Location);
            Owner->Component->ReportEventSpawn(EventGenInfo.CustomName, Owner->EmitterTime, ParticleLocation, FVector(NewParticle->Velocity),
                EventGenInfo.ParticleModuleEventsToSendToGame);
            bProcessed = true;
        }
    }
    return bProcessed;
}

bool UParticleModuleEventGenerator::HandleParticleKilled(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload,
    FBaseParticle* DeadParticle) {
    EventPayload->DeathTrackingCount++;
    bool bProcessed = false;
    for (FParticleEvent_GenerateInfo& EventGenInfo : Events) {
        if (EventGenInfo.Type != EPET_Death)
            continue;
        if (EventGenInfo.Frequency == 0 || (EventPayload->DeathTrackingCount % EventGenInfo.Frequency) == 0) {
            const FVector ParticleLocation = EventGenInfo.bUseOrbitOffset ? Owner->GetParticleLocationWithOrbitOffset(DeadParticle) : FVector(DeadParticle->Location);
            Owner->Component->ReportEventDeath(EventGenInfo.CustomName, Owner->EmitterTime, ParticleLocation, FVector(DeadParticle->Velocity),
                EventGenInfo.ParticleModuleEventsToSendToGame, DeadParticle->RelativeTime);
            bProcessed = true;
        }
    }
    return bProcessed;
}

bool UParticleModuleEventGenerator::HandleParticleCollision(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload,
    FParticleCollisionPayload* CollidePayload, FHitResult* Hit, FBaseParticle* CollideParticle, FVector& CollideDirection) {
    EventPayload->CollisionTrackingCount++;
    bool bProcessed = false;
    for (FParticleEvent_GenerateInfo& EventGenInfo : Events) {
        if (EventGenInfo.Type != EPET_Collision)
            continue;
        if (EventGenInfo.FirstTimeOnly) {
            if ((CollideParticle->Flags & STATE_Particle_CollisionHasOccurred) != 0)
                continue;
        } else if (EventGenInfo.LastTimeOnly) {
            if (CollidePayload->UsedCollisions != 0)
                continue;
        }
        if (EventGenInfo.Frequency == 0 || (EventPayload->CollisionTrackingCount % EventGenInfo.Frequency) == 0) {
            Owner->Component->ReportEventCollision(EventGenInfo.CustomName, Owner->EmitterTime, Hit->Location, CollideDirection,
                FVector(CollideParticle->Velocity), EventGenInfo.ParticleModuleEventsToSendToGame, CollideParticle->RelativeTime, Hit->Normal,
                Hit->Time, Hit->Item, Hit->BoneName, Hit->PhysMaterial.Get());
            bProcessed = true;
        }
    }
    return bProcessed;
}

bool UParticleModuleEventGenerator::HandleParticleBurst(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload,
    const int32 ParticleCount) {
    ++EventPayload->BurstTrackingCount;
    bool bProcessed = false;
    for (FParticleEvent_GenerateInfo& EventGenInfo : Events) {
        if (EventGenInfo.Type != EPET_Burst)
            continue;
        if (EventGenInfo.Frequency == 0 || (EventPayload->BurstTrackingCount % EventGenInfo.Frequency) == 0) {
            Owner->Component->ReportEventBurst(EventGenInfo.CustomName, Owner->EmitterTime, ParticleCount, Owner->Location,
                EventGenInfo.ParticleModuleEventsToSendToGame);
            bProcessed = true;
        }
    }
    return bProcessed;
}

UParticleModuleEventReceiverKillParticles::UParticleModuleEventReceiverKillParticles(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

bool UParticleModuleEventReceiverKillParticles::ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) {
    if (InEvent.EventName != EventName || (EventGeneratorType != EPET_Any && EventGeneratorType != InEvent.Type))
        return false;
    Owner->KillParticlesForced(true);
    if (bStopSpawning) {
        Owner->SetHaltSpawning(true);
        Owner->SetHaltSpawningExternal(true);
    }
    return true;
}

UParticleModuleEventReceiverSpawn::UParticleModuleEventReceiverSpawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

bool UParticleModuleEventReceiverSpawn::ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) {
    if (InEvent.EventName != EventName || (EventGeneratorType != EPET_Any && EventGeneratorType != InEvent.Type))
        return false;
    int32 Count = 0;
    switch (InEvent.Type) {
    case EPET_Spawn:
    case EPET_Burst:
    case EPET_Blueprint:
        Count = FMath::RoundToInt(SpawnCount.GetValue(InEvent.EmitterTime));
        break;
    case EPET_Death: {
        const FParticleEventDeathData& DeathData = static_cast<const FParticleEventDeathData&>(InEvent);
        Count = FMath::RoundToInt(SpawnCount.GetValue(bUseParticleTime ? DeathData.ParticleTime : InEvent.EmitterTime));
        break;
    }
    case EPET_Collision: {
        const FParticleEventCollideData& CollideData = static_cast<const FParticleEventCollideData&>(InEvent);
        UPhysicalMaterial* PhysMat = CollideData.PhysMat;
        bool bPhysMatInList = false;
        for (UPhysicalMaterial* Material : PhysicalMaterials)
            bPhysMatInList |= Material == PhysMat;
        const bool bPhysMatIsAllowed = !PhysMat || PhysicalMaterials.Num() == 0 || bPhysMatInList == !bBanPhysicalMaterials;
        if (bPhysMatIsAllowed)
            Count = FMath::RoundToInt(SpawnCount.GetValue(bUseParticleTime ? CollideData.ParticleTime : InEvent.EmitterTime));
        break;
    }
    default:
        break;
    }
    if (Count > 0) {
        FVector SpawnLocation = bUsePSysLocation ? Owner->Location : InEvent.Location;
        FVector Velocity = bInheritVelocity ? InEvent.Velocity * InheritVelocityScale.GetValue(InEvent.EmitterTime) : FVector::ZeroVector;
        Owner->ForceSpawn(DeltaTime, 0, Count, SpawnLocation, Velocity);
    }
    return true;
}

}
