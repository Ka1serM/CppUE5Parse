#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

void FCompactPose::ResetToAdditiveIdentity() {
    for (FTransform& Bone : Bones) Bone.SetIdentityZeroScale();
}

void FCompactPose::NormalizeRotations() {
    for (FTransform& Bone : Bones) Bone.NormalizeRotation();
}

bool FCompactPose::IsNormalized() const {
    for (const FTransform& Bone : Bones)
        if (!Bone.IsRotationNormalized()) return false;
    return true;
}

}
