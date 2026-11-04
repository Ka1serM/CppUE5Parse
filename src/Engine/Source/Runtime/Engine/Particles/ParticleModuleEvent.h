#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

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

UCLASS(abstract)
class UParticleModuleEventSendToGame : public UObject {
    GENERATED_BODY()
public:
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

UCLASS(abstract)
class UParticleModuleEventBase : public UParticleModule {
    GENERATED_BODY()
public:
    EModuleType GetModuleType() const override { return EPMT_Event; }
};

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

UCLASS()
class UParticleModuleEventReceiverKillParticles : public UParticleModuleEventReceiverBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleEventReceiverKillParticles(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bStopSpawning = false;

    bool ProcessParticleEvent(FParticleEmitterInstance* Owner, FParticleEventData& InEvent, float DeltaTime) override;
};

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
