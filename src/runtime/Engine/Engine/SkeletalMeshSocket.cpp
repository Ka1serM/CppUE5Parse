#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"

#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

bool USkeletalMeshSocket::GetSocketMatrixWithOffset(FMatrix& OutMatrix, USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const {
    const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
    if (BoneIndex == INDEX_NONE) return false;
    FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
    FMatrix RelSocketMatrix = GetSocketLocalTransform().ToMatrixWithScale();
    FMatrix RelOffsetMatrix = FTransform(InRotation, InOffset).ToMatrixWithScale();
    OutMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
    return true;
}

}
