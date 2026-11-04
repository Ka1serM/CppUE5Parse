#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLifetime.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleLifetimeBase : public UParticleModule {
    GENERATED_BODY()
public:
    virtual float GetMaxLifetime() { return 0.0f; }
    virtual float GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = nullptr) { return 0.0f; }
};

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

}
