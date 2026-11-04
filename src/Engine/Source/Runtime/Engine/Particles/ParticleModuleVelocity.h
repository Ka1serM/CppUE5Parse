#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleVelocity.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleVelocityBase : public UParticleModule {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bInWorldSpace = false;
    UPROPERTY()
    bool bApplyOwnerScale = false;
};

UCLASS()
class UParticleModuleVelocity : public UParticleModuleVelocityBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleVelocity(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartVelocity;
    UPROPERTY()
    FRawDistributionFloat StartVelocityRadial;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

UCLASS()
class UParticleModuleVelocity_Seeded : public UParticleModuleVelocity {
    GENERATED_BODY()
public:
    explicit UParticleModuleVelocity_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

UCLASS()
class UParticleModuleVelocityInheritParent : public UParticleModuleVelocityBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleVelocityInheritParent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector Scale;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
};

UCLASS()
class UParticleModuleVelocityOverLifetime : public UParticleModuleVelocityBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleVelocityOverLifetime(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector VelOverLife;
    UPROPERTY()
    bool Absolute = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

UCLASS()
class UParticleModuleVelocityCone : public UParticleModuleVelocityBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleVelocityCone(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat Angle;
    UPROPERTY()
    FRawDistributionFloat Velocity;
    UPROPERTY()
    FVector Direction = FVector(0.0f, 0.0f, 1.0f);

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

}
