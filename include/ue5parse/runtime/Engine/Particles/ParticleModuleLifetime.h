#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLifetime.generated.h"

namespace ue5parse::runtime {

// UParticleModuleLifetimeBase (UE's Particles/Lifetime/ParticleModuleLifetimeBase.h).
UCLASS(abstract)
class UParticleModuleLifetimeBase : public UParticleModule {
    GENERATED_BODY()
public:
    virtual float GetMaxLifetime() { return 0.0f; }
    virtual float GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = nullptr) { return 0.0f; }
};

// UParticleModuleLifetime (UE's Particles/Lifetime/ParticleModuleLifetime.h).
UCLASS()
class UParticleModuleLifetime : public UParticleModuleLifetimeBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLifetime(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat Lifetime;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    float GetMaxLifetime() override;
    float GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = nullptr) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleLifetime_Seeded (UE's Particles/Lifetime/ParticleModuleLifetime_Seeded.h).
UCLASS()
class UParticleModuleLifetime_Seeded : public UParticleModuleLifetime {
    GENERATED_BODY()
public:
    explicit UParticleModuleLifetime_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
    float GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = nullptr) override;
};

// UParticleModulePreciseLifetime: Riot module. It stores the absolute death time (emitter seconds) per particle, and
// the emitter instance kills on that time instead of RelativeTime > 1. Rebuilt from the 13.08 game binary.
UCLASS()
class UParticleModulePreciseLifetime : public UParticleModuleLifetimeBase {
    GENERATED_BODY()
public:
    explicit UParticleModulePreciseLifetime(const FObjectInitializer& ObjectInitializer);

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    // Riot's module returns 4 here too (one function serves both slots); the block is unused but moves the later blocks.
    uint32 RequiredBytesPerInstance() override { return 4; }
};

}
