#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleOrientation.h"

namespace ue5parse::runtime {

UParticleModuleOrientationAxisLock::UParticleModuleOrientationAxisLock(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

}
