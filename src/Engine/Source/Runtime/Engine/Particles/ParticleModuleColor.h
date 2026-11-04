#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleColor.generated.h"

namespace ue5parse::runtime {

class UInterpCurveEdSetup;
class UParticleEmitter;
struct FCurveEdEntry;
struct FRawDistributionFloat;
struct FRawDistributionVector;

UCLASS(abstract)
class UParticleModuleColorBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleColor : public UParticleModuleColorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleColor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartColor;
    UPROPERTY()
    FRawDistributionFloat StartAlpha;
    UPROPERTY()
    bool bClampAlpha = true;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

UCLASS()
class UParticleModuleColor_Seeded : public UParticleModuleColor {
    GENERATED_BODY()
public:
    explicit UParticleModuleColor_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

UCLASS()
class UParticleModuleColorOverLife : public UParticleModuleColorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleColorOverLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector ColorOverLife;
    UPROPERTY()
    FRawDistributionFloat AlphaOverLife;
    UPROPERTY()
    bool bClampAlpha = true;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

UCLASS()
class UParticleModuleColorScaleOverLife : public UParticleModuleColorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleColorScaleOverLife(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector ColorScaleOverLife;
    UPROPERTY()
    FRawDistributionFloat AlphaScaleOverLife;
    UPROPERTY()
    bool bEmitterTime = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
