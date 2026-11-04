#include "Engine/Source/Runtime/Engine/Animation/BoneSocketReference.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimInstanceProxy.h"
#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAsset.h"

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
