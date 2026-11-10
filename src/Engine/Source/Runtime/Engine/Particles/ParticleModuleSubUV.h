#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Engine/Source/Runtime/Engine/Particles/SubUVAnimation.h"

#include "ParticleModuleSubUV.generated.h"

namespace ue5parse::runtime {

class UParticleLODLevel;
class UParticleModuleTypeDataBase;
class USubUVAnimation;

UCLASS(abstract)
class UParticleModuleSubUVBase : public UParticleModule {
    GENERATED_BODY()
public:
    EModuleType GetModuleType() const override { return EPMT_SubUV; }
};

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
