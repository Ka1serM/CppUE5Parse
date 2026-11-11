#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLifetime.h"

#include "ParticleModulePreciseLifetime.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UParticleModulePreciseLifetime : public UParticleModuleLifetimeBase {
    GENERATED_BODY()
public:
    explicit UParticleModulePreciseLifetime(const FObjectInitializer& ObjectInitializer);

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override { return 4; }
    //! Riot's module returns 4 here too: one function serves both slots, and the unused block moves the later blocks.
    uint32 RequiredBytesPerInstance() override { return 4; }
};

}
