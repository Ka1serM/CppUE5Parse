#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleLocation.h"

#include "ParticleModuleSourceMovement.generated.h"

namespace ue5parse::runtime {

// UParticleModuleSourceMovement (UE's Particles/Location/ParticleModuleSourceMovement.h).
UCLASS()
class UParticleModuleSourceMovement : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleSourceMovement(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector SourceMovementScale;

    void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
