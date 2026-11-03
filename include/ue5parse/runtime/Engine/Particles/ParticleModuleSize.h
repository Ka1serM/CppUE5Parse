#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleSize.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;
class UParticleLODLevel;

// UParticleModuleSizeBase (UE's Particles/Size/ParticleModuleSizeBase.h).
UCLASS(abstract)
class UParticleModuleSizeBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleSize (UE's Particles/Size/ParticleModuleSize.h).
UCLASS()
class UParticleModuleSize : public UParticleModuleSizeBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSize(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartSize;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleSize_Seeded (UE's Particles/Size/ParticleModuleSize_Seeded.h).
UCLASS()
class UParticleModuleSize_Seeded : public UParticleModuleSize {
    GENERATED_BODY()
public:
    explicit UParticleModuleSize_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModuleSizeMultiplyLife (UE's Particles/Size/ParticleModuleSizeMultiplyLife.h).
UCLASS()
class UParticleModuleSizeMultiplyLife : public UParticleModuleSizeBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSizeMultiplyLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector LifeMultiplier;
    UPROPERTY()
    bool MultiplyX = true;
    UPROPERTY()
    bool MultiplyY = true;
    UPROPERTY()
    bool MultiplyZ = true;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    bool IsSizeMultiplyLife() override { return true; }
};

// UParticleModuleSizeScale (UE's Particles/Size/ParticleModuleSizeScale.h).
UCLASS()
class UParticleModuleSizeScale : public UParticleModuleSizeBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSizeScale(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector SizeScale;
    UPROPERTY()
    bool EnableX = true;
    UPROPERTY()
    bool EnableY = true;
    UPROPERTY()
    bool EnableZ = true;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

// UParticleModuleSizeScaleBySpeed (UE's Particles/Size/ParticleModuleSizeScaleBySpeed.h).
UCLASS()
class UParticleModuleSizeScaleBySpeed : public UParticleModuleSizeBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSizeScaleBySpeed(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector2D SpeedScale;
    UPROPERTY()
    FVector2D MaxScale;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
