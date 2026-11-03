#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLight.generated.h"

namespace ue5parse::runtime {

// UParticleModuleLightBase (UE's Particles/Light/ParticleModuleLightBase.h).
UCLASS(abstract)
class UParticleModuleLightBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleLight: gives each particle a light payload (UE's Particles/Light/ParticleModuleLight.h). The high quality
// lights of UE are point light components; the recording exports their payload with bHighQuality instead.
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

// UParticleModuleLight_Seeded (UE's Particles/Light/ParticleModuleLight_Seeded.h).
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
