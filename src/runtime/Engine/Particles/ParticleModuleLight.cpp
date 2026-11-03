#include "ue5parse/runtime/Engine/Particles/ParticleModuleLight.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"

// UE's ParticleModules.cpp (UE 5.3), the light modules. UE spawns a point light component for each high quality light;
// the runtime keeps the payload only, and the recording exports it with bHighQuality.

namespace ue5parse::runtime {

// UE's fx.ParticleDefaultLightInverseExposureBlend default. r.ParticleLightQuality is 2 (all lights).
static constexpr float ParticleDefaultLightInverseExposureBlend = 0.0f;

UParticleModuleLight::UParticleModuleLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bCurvesAsColor = true;
    bUseInverseSquaredFalloff = true;
    SpawnFraction = 1;
    bSupported3DDrawMode = true;
    b3DDrawMode = true;
    VolumetricScatteringIntensity = 0.0f;
}

void UParticleModuleLight::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLight::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    PARTICLE_ELEMENT(FLightParticlePayload, LightData);
    const float Brightness = BrightnessOverLife.GetValue(Particle.RelativeTime, Owner->Component, InRandomStream);
    LightData.ColorScale = FVector3f(ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component, 0, InRandomStream)) * Brightness;
    LightData.RadiusScale = RadiusScale.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    LightData.LightExponent = bUseInverseSquaredFalloff ? 0 : LightExponent.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    LightData.InverseExposureBlend = bOverrideInverseExposureBlend ? InverseExposureBlend : ParticleDefaultLightInverseExposureBlend;
    const float RandomNumber = InRandomStream->GetFraction();
    LightData.bValid = RandomNumber < SpawnFraction;
    LightData.bAffectsTranslucency = bAffectsTranslucency;
    LightData.bHighQuality = bHighQualityLights;
    LightData.LightId = 0;
}

void UParticleModuleLight::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if (Owner == nullptr || Owner->ActiveParticles <= 0 || Owner->ParticleData == nullptr || Owner->ParticleIndices == nullptr)
        return;
    BEGIN_UPDATE_LOOP;
    {
        PARTICLE_ELEMENT(FLightParticlePayload, Data);
        const float Brightness = BrightnessOverLife.GetValue(Particle.RelativeTime, Owner->Component);
        Data.ColorScale = FVector3f(ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component)) * Brightness;
    }
    END_UPDATE_LOOP;
}

uint32 UParticleModuleLight::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FLightParticlePayload);
}

UParticleModuleLight_Seeded::UParticleModuleLight_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLight_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

}
