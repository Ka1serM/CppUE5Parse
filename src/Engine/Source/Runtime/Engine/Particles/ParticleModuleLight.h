#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLight.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleLightBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleLight : public UParticleModuleLightBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bUseInverseSquaredFalloff = true;
    UPROPERTY()
    bool bAffectsTranslucency = false;
    UPROPERTY()
    bool bOverrideInverseExposureBlend = false;
    UPROPERTY()
    bool bPreviewLightRadius = false;
    UPROPERTY()
    float SpawnFraction = 1.0f;
    UPROPERTY()
    FRawDistributionVector ColorScaleOverLife;
    UPROPERTY()
    FRawDistributionFloat BrightnessOverLife;
    UPROPERTY()
    FRawDistributionFloat RadiusScale;
    UPROPERTY()
    FRawDistributionFloat LightExponent;
    UPROPERTY()
    float InverseExposureBlend = 0.0f;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    float VolumetricScatteringIntensity = 0.0f;
    UPROPERTY()
    bool bHighQualityLights = false;
    UPROPERTY()
    bool bShadowCastingLights = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    EModuleType GetModuleType() const override { return EPMT_Light; }
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

UCLASS()
class UParticleModuleLight_Seeded : public UParticleModuleLight {
    GENERATED_BODY()
public:
    explicit UParticleModuleLight_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

}
