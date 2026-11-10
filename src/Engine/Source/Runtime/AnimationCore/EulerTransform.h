#pragma once

#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "EulerTransform.generated.h"

namespace ue5parse::runtime {

enum class EEulerRotationOrder : uint8 { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

USTRUCT()
struct FEulerTransform {
    GENERATED_BODY()
    UPROPERTY()
    FVector Location = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY()
    FVector Scale = FVector::OneVector;

    FEulerTransform() = default;
    FEulerTransform(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale)
        : Location(InLocation), Rotation(InRotation), Scale(InScale) {}
    explicit FEulerTransform(const FTransform& InTransform)
        : Location(InTransform.GetLocation()), Rotation(InTransform.GetRotation().Rotator()), Scale(InTransform.GetScale3D()) {}
};

}
