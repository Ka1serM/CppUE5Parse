#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "Attenuation.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EAttenuationDistanceModel : uint8 {
    Linear,
    Logarithmic,
    Inverse,
    LogReverse,
    NaturalSound,
    Custom,
};

namespace EAttenuationShape {

UENUM()
enum Type : uint8 { Sphere, Capsule, Box, Cone };

}

UENUM()
enum class ENaturalSoundFalloffMode : uint8 {
    Continues,
    Silent,
    Hold,
};

USTRUCT()
struct FBaseAttenuationSettings {
    GENERATED_BODY()
    UPROPERTY()
    EAttenuationDistanceModel DistanceAlgorithm = EAttenuationDistanceModel::Linear;
    UPROPERTY()
    FRuntimeFloatCurve CustomAttenuationCurve;
    UPROPERTY()
    TEnumAsByte<EAttenuationShape::Type> AttenuationShape = EAttenuationShape::Sphere;
    UPROPERTY()
    float dBAttenuationAtMax = -60.0f;
    UPROPERTY()
    ENaturalSoundFalloffMode FalloffMode = ENaturalSoundFalloffMode::Continues;
    UPROPERTY()
    FVector AttenuationShapeExtents = FVector(400.0, 0.0, 0.0);
    UPROPERTY()
    float ConeOffset = 0.0f;
    UPROPERTY()
    float FalloffDistance = 3600.0f;
    UPROPERTY()
    float ConeSphereRadius = 0.0f;
    UPROPERTY()
    float ConeSphereFalloffDistance = 0.0f;
};

}
