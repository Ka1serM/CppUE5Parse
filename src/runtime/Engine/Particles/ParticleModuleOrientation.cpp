#include "ue5parse/runtime/Engine/Particles/ParticleModuleOrientation.h"

// UE's ParticleModules.cpp (UE 5.3), the orientation module.

namespace ue5parse::runtime {

UParticleModuleOrientationAxisLock::UParticleModuleOrientationAxisLock(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

}
