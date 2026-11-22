#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/BoundingVolumeHierarchy.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"

namespace ue5parse::runtime::Chaos {

class FBVHParticles {
public:
    void Serialize(FChaosArchive& Ar);

    const TArray<FVector>& GetX() const { return MX; }
    const TBoundingVolumeHierarchy<TArray<int32>, double>& GetBVH() const { return MBVH; }

private:
    TArray<FVector> MX;
    TBoundingVolumeHierarchy<TArray<int32>, double> MBVH;
};

using FBVHParticlesFloat3 = FBVHParticles;

}
