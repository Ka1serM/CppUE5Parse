#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleOrientation.generated.h"

namespace ue5parse::runtime {

// UParticleModuleOrientationBase (UE's Particles/Orientation/ParticleModuleOrientationBase.h).
UCLASS(abstract)
class UParticleModuleOrientationBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleOrientationAxisLock: the emitter reads LockAxisFlags when it caches the module info; the module does
// nothing per particle (UE's Particles/Orientation/ParticleModuleOrientationAxisLock.h).
UCLASS()
class UParticleModuleOrientationAxisLock : public UParticleModuleOrientationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleOrientationAxisLock(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<EParticleAxisLock> LockAxisFlags = EPAL_NONE;
};

}
