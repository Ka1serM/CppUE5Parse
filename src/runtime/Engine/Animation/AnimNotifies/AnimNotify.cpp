#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

void UAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) {
    USkeletalMeshComponent* PrevContext = MeshContext;
    MeshContext = MeshComp;
    Notify(MeshComp, Animation);
    Received_Notify(MeshComp, Animation, EventReference);
    MeshContext = PrevContext;
}

void UAnimNotify::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) {
    const FAnimNotifyEventReference EventReference;
    Notify(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset, EventReference);
}

UWorld* UAnimNotify::GetWorld() const { return MeshContext ? MeshContext->GetWorld() : nullptr; }

}
