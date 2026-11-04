#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNotifies/AnimNotify_PlayMontageNotify.h"

#include "Engine/Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

namespace {

void Broadcast(const FBranchingPointNotifyPayload& Payload, FMulticastScriptDelegate UAnimInstance::*Delegate, FName NotifyName) {
    USkeletalMeshComponent* MeshComp = Payload.SkelMeshComponent;
    UAnimInstance* AnimInstance = MeshComp != nullptr ? MeshComp->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr) return;
    struct FPlayMontageAnimNotifyDelegate_Parms {
        FName NotifyName;
        FBranchingPointNotifyPayload BranchingPointPayload;
    } Parms{NotifyName, Payload};
    (AnimInstance->*Delegate).ProcessMulticastDelegate(&Parms);
}

}

UAnimNotify_PlayMontageNotify::UAnimNotify_PlayMontageNotify(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bIsNativeBranchingPoint = true;
}

void UAnimNotify_PlayMontageNotify::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) {
    Super::BranchingPointNotify(BranchingPointPayload);
    Broadcast(BranchingPointPayload, &UAnimInstance::OnPlayMontageNotifyBegin, NotifyName);
}

UAnimNotify_PlayMontageNotifyWindow::UAnimNotify_PlayMontageNotifyWindow(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bIsNativeBranchingPoint = true;
}

void UAnimNotify_PlayMontageNotifyWindow::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) {
    Super::BranchingPointNotifyBegin(BranchingPointPayload);
    Broadcast(BranchingPointPayload, &UAnimInstance::OnPlayMontageNotifyBegin, NotifyName);
}

void UAnimNotify_PlayMontageNotifyWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) {
    Super::BranchingPointNotifyEnd(BranchingPointPayload);
    Broadcast(BranchingPointPayload, &UAnimInstance::OnPlayMontageNotifyEnd, NotifyName);
}

}
