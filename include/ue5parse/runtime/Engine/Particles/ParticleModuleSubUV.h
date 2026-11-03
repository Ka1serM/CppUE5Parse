#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"
#include "ue5parse/runtime/Engine/Particles/SubUVAnimation.h"
#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleModuleSubUV.generated.h"

namespace ue5parse::runtime {

class UParticleLODLevel;
class UParticleModuleTypeDataBase;
class USubUVAnimation;

// UParticleModuleSubUVBase (UE's Particles/SubUV/ParticleModuleSubUVBase.h).
UCLASS(abstract)
class UParticleModuleSubUVBase : public UParticleModule {
    GENERATED_BODY()
public:
    EModuleType GetModuleType() const override { return EPMT_SubUV; }
};

// UParticleModuleSubUV (UE's Particles/SubUV/ParticleModuleSubUV.h).
UCLASS()
class UParticleModuleSubUV : public UParticleModuleSubUVBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSubUV(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USubUVAnimation> Animation = nullptr;
    UPROPERTY()
    FRawDistributionFloat SubImageIndex;
    UPROPERTY()
    bool bUseRealTime = false;

    void PostLoad() override;
    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    virtual float DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle, EParticleSubUVInterpMethod InterpMethod,
        FFullSubUVPayload& SubUVPayload, float DeltaTime);
};

// UParticleModuleSubUVMovie (UE's Particles/SubUV/ParticleModuleSubUVMovie.h).
UCLASS()
class UParticleModuleSubUVMovie : public UParticleModuleSubUV {
    GENERATED_BODY()
public:
    explicit UParticleModuleSubUVMovie(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bUseEmitterTime = false;
    UPROPERTY()
    FRawDistributionFloat FrameRate;
    UPROPERTY()
    int32 StartingFrame = 1;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    float DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle, EParticleSubUVInterpMethod InterpMethod,
        FFullSubUVPayload& SubUVPayload, float DeltaTime) override;
};

}
