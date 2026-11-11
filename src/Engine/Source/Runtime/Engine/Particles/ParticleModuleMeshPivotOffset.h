#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"

#include "ParticleModuleMeshPivotOffset.generated.h"

namespace ue5parse::runtime {

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
