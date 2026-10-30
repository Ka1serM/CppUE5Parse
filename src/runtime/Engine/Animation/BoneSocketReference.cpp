#include "ue5parse/runtime/Engine/Animation/BoneSocketReference.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAsset.h"

// UE's Animation/BoneSocketReference.cpp (UE 5.3).

namespace ue5parse::runtime {

void FSocketReference::InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy) {
    InvalidateCachedBoneIndex();
    if (SocketName.isNone()) return;
    const USkeletalMeshComponent* OwnerMeshComponent = InAnimInstanceProxy->GetSkelMeshComponent();
    const USkinnedAsset* Asset = OwnerMeshComponent ? OwnerMeshComponent->GetSkinnedAsset() : nullptr;
    const USkeletalMeshSocket* Socket = Asset ? Asset->FindSocket(SocketName) : nullptr;
    if (Socket == nullptr) {
        ReportLoadProblem("anim.socket_missing", NameToString(SocketName));
        return;
    }
    CachedSocketLocalTransform = Socket->GetSocketLocalTransform();
    CachedSocketMeshBoneIndex = FMeshPoseBoneIndex(Asset->GetRefSkeleton().FindBoneIndex(Socket->BoneName));
}

void FSocketReference::InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones) {
    if (!CachedSocketMeshBoneIndex.IsValid()) return;
    CachedSocketCompactBoneIndex = RequiredBones.MakeCompactPoseIndex(CachedSocketMeshBoneIndex);
}

}
