#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"

#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

void USkeletalMeshSocket::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(4);
    Super::Serialize(Ar);
}

bool USkeletalMeshSocket::GetSocketMatrixWithOffset(FMatrix& OutMatrix, USkeletalMeshComponent* SkelComp, const FVector& InOffset,
                                                    const FRotator& InRotation) const {
    const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
    if (BoneIndex == INDEX_NONE)
        return false;
    FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
    FMatrix RelSocketMatrix = GetSocketLocalTransform().ToMatrixWithScale();
    FMatrix RelOffsetMatrix = FTransform(InRotation, InOffset).ToMatrixWithScale();
    OutMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
    return true;
}

}
