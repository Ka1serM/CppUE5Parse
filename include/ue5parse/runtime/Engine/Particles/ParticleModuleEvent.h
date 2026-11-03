#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleEvent.generated.h"

namespace ue5parse::runtime {

struct FParticleEventData;
struct FParticleEventInstancePayload;
struct FParticleCollisionPayload;
struct FHitResult;
class UPhysicalMaterial;

UENUM()
enum EParticleEventType : int {
    EPET_Any,
    EPET_Spawn,
    EPET_Death,
    EPET_Collision,
    EPET_Burst,
    EPET_Blueprint,
    EPET_MAX,
};

// UParticleModuleEventSendToGame: a game callback of a particle event (UE's
// Particles/Event/ParticleModuleEventSendToGame.h).
UCLASS(abstract)
class UParticleModuleEventSendToGame : public UObject {
    GENERATED_BODY()
public:
    virtual void DoEvent(const FVector& InCollideDirection, const FVector& InHitLocation, const FVector& InHitNormal, const FName& InBoneName) {}
};

USTRUCT()
struct FParticleEvent_GenerateInfo {
    GENERATED_BODY()

    UPROPERTY()
    TEnumAsByte<EParticleEventType> Type = EPET_Any;
    UPROPERTY()
    int32 Frequency = 0;
    UPROPERTY()
    int32 ParticleFrequency = 0;
    UPROPERTY()
    bool FirstTimeOnly = false;
    UPROPERTY()
    bool LastTimeOnly = false;
    UPROPERTY()
    bool UseReflectedImpactVector = false;
    UPROPERTY()
    bool bUseOrbitOffset = false;
    UPROPERTY()
    FName CustomName;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModuleEventSendToGame>> ParticleModuleEventsToSendToGame;
};

// UParticleModuleEventBase (UE's Particles/Event/ParticleModuleEventBase.h).
UCLASS(abstract)
class UParticleModuleEventBase : public UParticleModule {
    GENERATED_BODY()
public:
    EModuleType GetModuleType() const override { return EPMT_Event; }
};

// UParticleModuleEventGenerator: reports the spawn, death, collision and burst events of the particles to the component
// (UE's Particles/Event/ParticleModuleEventGenerator.h).
UCLASS()
class UParticleModuleEventGenerator : public UParticleModuleEventBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleEventGenerator(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FParticleEvent_GenerateInfo> Events;

    uint32 RequiredBytesPerInstance() override;
    uint32 PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) override;
    bool HandleParticleSpawned(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload, FBaseParticle* NewParticle);
    bool HandleParticleKilled(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload, FBaseParticle* DeadParticle);
    bool HandleParticleCollision(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload, FParticleCollisionPayload* CollidePayload,
        FHitResult* Hit, FBaseParticle* CollideParticle, FVector& CollideDirection);
    bool HandleParticleBurst(FParticleEmitterInstance* Owner, FParticleEventInstancePayload* EventPayload, const int32 ParticleCount);
};

// UParticleModuleEventReceiverBase: a module that acts on the events of the other emitters of the system (UE's
// Particles/Event/ParticleModuleEventReceiverBase.h).
UCLASS(abstract)
class UParticleModuleEventReceiverBase : public UParticleModuleEventBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EParticleEventType> EventGeneratorType = EPET_Any;
    UPROPERTY()
    FName EventName;

    virtual bool WillProcessParticleEvent(EParticleEventType InEventType) { return EventGeneratorType == EPET_Any || InEventType == EventGeneratorType; }
    virtual bool ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) { return false; }
};

// UParticleModuleEventReceiverKillParticles (UE's Particles/Event/ParticleModuleEventReceiverKillParticles.h).
UCLASS()
class UParticleModuleEventReceiverKillParticles : public UParticleModuleEventReceiverBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleEventReceiverKillParticles(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bStopSpawning = false;

    bool ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) override;
};

// UParticleModuleEventReceiverSpawn: spawns particles at the events (UE's
// Particles/Event/ParticleModuleEventReceiverSpawn.h).
UCLASS()
class UParticleModuleEventReceiverSpawn : public UParticleModuleEventReceiverBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleEventReceiverSpawn(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat SpawnCount;
    UPROPERTY()
    bool bUseParticleTime = false;
    UPROPERTY()
    bool bUsePSysLocation = false;
    UPROPERTY()
    bool bInheritVelocity = false;
    UPROPERTY()
    FRawDistributionVector InheritVelocityScale;
    UPROPERTY()
    TArray<TObjectPtr<UPhysicalMaterial>> PhysicalMaterials;
    UPROPERTY()
    bool bBanPhysicalMaterials = false;

    bool ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) override;
};

}
