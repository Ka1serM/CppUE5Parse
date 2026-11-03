#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleRotationRate.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;

// UParticleModuleRotationRateBase (UE's Particles/RotationRate/ParticleModuleRotationRateBase.h).
UCLASS(abstract)
class UParticleModuleRotationRateBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleRotationRate (UE's Particles/RotationRate/ParticleModuleRotationRate.h).
UCLASS()
class UParticleModuleRotationRate : public UParticleModuleRotationRateBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotationRate(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat StartRotationRate;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleRotationRate_Seeded (UE's Particles/RotationRate/ParticleModuleRotationRate_Seeded.h).
UCLASS()
class UParticleModuleRotationRate_Seeded : public UParticleModuleRotationRate {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotationRate_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModuleRotationRateMultiplyLife (UE's Particles/RotationRate/ParticleModuleRotationRateMultiplyLife.h).
UCLASS()
class UParticleModuleRotationRateMultiplyLife : public UParticleModuleRotationRateBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotationRateMultiplyLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat LifeMultiplier;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

// UParticleModuleMeshRotationRate (UE's Particles/RotationRate/ParticleModuleMeshRotationRate.h).
UCLASS()
class UParticleModuleMeshRotationRate : public UParticleModuleRotationRateBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotationRate(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartRotationRate;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    bool TouchesMeshRotation() const override { return true; }
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleMeshRotationRate_Seeded (UE's Particles/RotationRate/ParticleModuleMeshRotationRate_Seeded.h).
UCLASS()
class UParticleModuleMeshRotationRate_Seeded : public UParticleModuleMeshRotationRate {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotationRate_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModuleMeshRotationRateMultiplyLife (UE's Particles/RotationRate/ParticleModuleMeshRotationRateMultiplyLife.h).
UCLASS()
class UParticleModuleMeshRotationRateMultiplyLife : public UParticleModuleRotationRateBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotationRateMultiplyLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector LifeMultiplier;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

// UParticleModuleMeshRotationRateOverLife (UE's Particles/RotationRate/ParticleModuleMeshRotationRateOverLife.h).
UCLASS()
class UParticleModuleMeshRotationRateOverLife : public UParticleModuleRotationRateBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotationRateOverLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector RotRate;
    UPROPERTY()
    bool bScaleRotRate = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    bool TouchesMeshRotation() const override { return true; }
};

}
