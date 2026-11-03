#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleModuleSpawn.generated.h"

namespace ue5parse::runtime {

// UParticleModuleSpawnBase: a module that changes how many particles the emitter spawns (UE's
// Particles/Spawn/ParticleModuleSpawnBase.h).
UCLASS(abstract)
class UParticleModuleSpawnBase : public UParticleModule {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bProcessSpawnRate = true;
    UPROPERTY()
    bool bProcessBurstList = true;

    EModuleType GetModuleType() const override { return EPMT_Spawn; }
    // Sets the number of particles to spawn this frame; false when the emitter must not add its own spawn rate.
    virtual bool GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& Rate) {
        return bProcessSpawnRate;
    }
    virtual bool GetBurstCount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number) {
        Number = 0;
        return false;
    }
    virtual float GetMaximumSpawnRate() { return 0.0f; }
    virtual float GetEstimatedSpawnRate() { return 0.0f; }
    virtual int32 GetMaximumBurstCount() { return 0; }
};

// UParticleModuleSpawn: the spawn rate and bursts of an emitter (UE's Particles/Spawn/ParticleModuleSpawn.h).
UCLASS()
class UParticleModuleSpawn : public UParticleModuleSpawnBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSpawn(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat Rate;
    UPROPERTY()
    FRawDistributionFloat RateScale;
    UPROPERTY()
    TArray<FParticleBurst> BurstList;
    UPROPERTY()
    FRawDistributionFloat BurstScale;
    UPROPERTY()
    TEnumAsByte<EParticleBurstMethod> ParticleBurstMethod = EPBM_Instant;
    UPROPERTY()
    bool bApplyGlobalSpawnRateScale = true;

    bool GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& InRate) override {
        return false;
    }
    float GetMaximumSpawnRate() override;
    float GetEstimatedSpawnRate() override;
    int32 GetMaximumBurstCount() override;
    // The spawn rate scale of r.EmitterSpawnRateScale, which is 1 by default.
    float GetGlobalRateScale() const { return 1.0f; }
};

// UParticleModuleSpawnPerUnit: spawns particles by the distance the emitter moves (UE's
// Particles/Spawn/ParticleModuleSpawnPerUnit.h).
UCLASS()
class UParticleModuleSpawnPerUnit : public UParticleModuleSpawnBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSpawnPerUnit(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float UnitScalar = 50.0f;
    UPROPERTY()
    float MovementTolerance = 0.1f;
    UPROPERTY()
    FRawDistributionFloat SpawnPerUnit;
    UPROPERTY()
    float MaxFrameDistance = 0;
    UPROPERTY()
    bool bIgnoreSpawnRateWhenMoving = false;
    UPROPERTY()
    bool bIgnoreMovementAlongX = false;
    UPROPERTY()
    bool bIgnoreMovementAlongY = false;
    UPROPERTY()
    bool bIgnoreMovementAlongZ = false;

    uint32 RequiredBytesPerInstance() override;
    bool GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& Rate) override;
};

}
