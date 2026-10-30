#include "ue5parse/runtime/AnimationCore/AnimationCoreLibrary.h"

namespace ue5parse::runtime::AnimationCore {

FQuat SolveAim(const FTransform& CurrentTransform, const FVector& TargetPosition, const FVector& AimVector, bool bUseUpVector,
               const FVector& UpVector, float AimClampInDegree) {
    if (!AimVector.IsNormalized() || (bUseUpVector && !UpVector.IsNormalized())) return FQuat::Identity;
    FVector ToTarget = TargetPosition - CurrentTransform.GetLocation();
    ToTarget.Normalize();
    if (AimClampInDegree > ZERO_ANIMWEIGHT_THRESH) {
        const double AimClampInRadians = FMath::DegreesToRadians(FMath::Min(double(AimClampInDegree), 180.0));
        const double DiffAngle = FMath::Acos(FVector::DotProduct(AimVector, ToTarget));
        if (DiffAngle > AimClampInRadians) {
            ToTarget = AimVector + (ToTarget - AimVector) * (AimClampInRadians / DiffAngle);
            ToTarget.Normalize();
        }
    }
    if (bUseUpVector) {
        ToTarget = FVector::VectorPlaneProject(ToTarget, UpVector);
        ToTarget.Normalize();
    }
    return FQuat::FindBetweenNormals(AimVector, ToTarget);
}

}
