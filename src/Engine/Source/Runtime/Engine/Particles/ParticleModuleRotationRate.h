#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleRotationRate.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;

UCLASS(abstract)
class UParticleModuleRotationRateBase : public UParticleModule {
    GENERATED_BODY()
};

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
