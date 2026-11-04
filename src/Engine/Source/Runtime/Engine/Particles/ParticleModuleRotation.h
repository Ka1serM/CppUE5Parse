#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleRotation.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleRotationBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleRotation : public UParticleModuleRotationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotation(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat StartRotation;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

UCLASS()
class UParticleModuleRotation_Seeded : public UParticleModuleRotation {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotation_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

UCLASS()
class UParticleModuleMeshRotation : public UParticleModuleRotationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotation(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartRotation;
    UPROPERTY()
    bool bInheritParent = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    bool TouchesMeshRotation() const override { return true; }
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

UCLASS()
class UParticleModuleMeshRotation_Seeded : public UParticleModuleMeshRotation {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshRotation_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

UCLASS()
class UParticleModuleRotationOverLifetime : public UParticleModuleRotationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleRotationOverLifetime(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat RotationOverLife;
    UPROPERTY()
    bool Scale = true;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
