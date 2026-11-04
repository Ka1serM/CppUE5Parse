#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleOrientation.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleOrientationBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleOrientationAxisLock : public UParticleModuleOrientationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleOrientationAxisLock(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<EParticleAxisLock> LockAxisFlags = EPAL_NONE;
};

}
