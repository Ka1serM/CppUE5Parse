#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"

#include "ParticleModuleSourceMovement.generated.h"

namespace ue5parse::runtime {

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
