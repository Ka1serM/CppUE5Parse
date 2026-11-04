#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleModuleSpawn.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleSpawnBase : public UParticleModule {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bProcessSpawnRate = true;
    UPROPERTY()
    bool bProcessBurstList = true;

    EModuleType GetModuleType() const override { return EPMT_Spawn; }
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
    float GetGlobalRateScale() const { return 1.0f; }
};

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
