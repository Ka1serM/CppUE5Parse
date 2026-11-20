#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/SHMath.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

template <int32 SHOrder>
class TVolumeLightingSample {
public:
    FVector3f Position;
    float Radius = 0.0f;
    TSHVectorRGB<SHOrder> Lighting;
    FColor PackedSkyBentNormal;
    float DirectionalLightShadowing = 1.0f;

    TVolumeLightingSample() = default;
    template <int32 OtherOrder>
    explicit TVolumeLightingSample(const TVolumeLightingSample<OtherOrder>& Other)
        : Position(Other.Position), Radius(Other.Radius), Lighting(Other.Lighting), PackedSkyBentNormal(Other.PackedSkyBentNormal),
          DirectionalLightShadowing(Other.DirectionalLightShadowing) {}
};

FArchive& operator<<(FArchive& Ar, TVolumeLightingSample<2>& Sample);
FArchive& operator<<(FArchive& Ar, TVolumeLightingSample<3>& Sample);

using FVolumeLightingSample = TVolumeLightingSample<3>;

class FPrecomputedLightVolumeData {
public:
    bool bInitialized = false;
    FBox Bounds;
    TArray<FVolumeLightingSample> HighQualitySamples;
    TArray<FVolumeLightingSample> LowQualitySamples;

    friend FArchive& operator<<(FArchive& Ar, FPrecomputedLightVolumeData& Volume);
};

FArchive& operator<<(FArchive& Ar, std::unique_ptr<FPrecomputedLightVolumeData>& Volume);

}
