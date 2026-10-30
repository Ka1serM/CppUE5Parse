#include "ue5parse/runtime/AnimGraphRuntime/AnimNotifies/AnimNotify_PlayMontageNotify.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

// UE's AnimNotify_PlayMontageNotify.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {

void Broadcast(const FBranchingPointNotifyPayload& Payload, FMulticastScriptDelegate UAnimInstance::*Delegate, FName NotifyName) {
    USkeletalMeshComponent* MeshComp = Payload.SkelMeshComponent;
    UAnimInstance* AnimInstance = MeshComp != nullptr ? MeshComp->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr) return;
    // The signature's parameters, the payload by value as in the generated parameter struct.
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
