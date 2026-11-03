#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModuleLocation.h"

#include "ParticleModuleMeshPivotOffset.generated.h"

namespace ue5parse::runtime {

// Riot's: the pivot of the mesh particles, which each particle keeps in its payload (VALORANT's Engine module).
UCLASS()
class UParticleModuleMeshPivotOffset : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector PivotOffset = FVector::ZeroVector;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override { return sizeof(FVector); }
};

}
