#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleRotation.generated.h"

namespace ue5parse::runtime {

// UParticleModuleRotationBase (UE's Particles/Rotation/ParticleModuleRotationBase.h).
UCLASS(abstract)
class UParticleModuleRotationBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleRotation (UE's Particles/Rotation/ParticleModuleRotation.h).
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

// UParticleModuleRotation_Seeded (UE's Particles/Rotation/ParticleModuleRotation_Seeded.h).
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

// UParticleModuleMeshRotation (UE's Particles/Rotation/ParticleModuleMeshRotation.h).
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

// UParticleModuleMeshRotation_Seeded (UE's Particles/Rotation/ParticleModuleMeshRotation_Seeded.h).
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

// UParticleModuleRotationOverLifetime (UE's Particles/Rotation/ParticleModuleRotationOverLifetime.h).
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
