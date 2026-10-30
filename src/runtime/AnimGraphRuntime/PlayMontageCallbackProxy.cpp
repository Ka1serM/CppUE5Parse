#include "ue5parse/runtime/AnimGraphRuntime/PlayMontageCallbackProxy.h"

#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {
namespace {

void BroadcastName(const FMulticastScriptDelegate& Delegate, FName Name) { Delegate.ProcessMulticastDelegate(&Name); }

}

UPlayMontageCallbackProxy::UPlayMontageCallbackProxy(const FObjectInitializer& ObjectInitializer) : UObject(ObjectInitializer) {}

UPlayMontageCallbackProxy* UPlayMontageCallbackProxy::CreateProxyObjectForPlayMontage(USkeletalMeshComponent* InSkeletalMeshComponent,
                                                                                     UAnimMontage* MontageToPlay, float PlayRate,
                                                                                     float StartingPosition, FName StartingSection) {
    UPlayMontageCallbackProxy* Proxy = NewObject<UPlayMontageCallbackProxy>(nullptr);
    Proxy->SetFlags(RF_StrongRefOnFrame);
    Proxy->PlayMontage(InSkeletalMeshComponent, MontageToPlay, PlayRate, StartingPosition, StartingSection);
    return Proxy;
}

bool UPlayMontageCallbackProxy::PlayMontage(USkeletalMeshComponent* InSkeletalMeshComponent, UAnimMontage* MontageToPlay, float PlayRate,
                                            float StartingPosition, FName StartingSection) {
    bool bPlayedSuccessfully = false;
    if (InSkeletalMeshComponent != nullptr) {
        if (UAnimInstance* AnimInstance = InSkeletalMeshComponent->GetAnimInstance()) {
            const float MontageLength =
                AnimInstance->Montage_Play(MontageToPlay, PlayRate, EMontagePlayReturnType::MontageLength, StartingPosition);
            bPlayedSuccessfully = MontageLength > 0.0f;

            if (bPlayedSuccessfully) {
                AnimInstancePtr = AnimInstance;
                if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay))
                    MontageInstanceID = MontageInstance->GetInstanceID();

                if (!StartingSection.isNone()) AnimInstance->Montage_JumpToSection(StartingSection, MontageToPlay);

                FOnMontageBlendingOutStarted BlendingOutDelegate = [this](UAnimMontage* Montage, bool bInterrupted) {
                    OnMontageBlendingOut(Montage, bInterrupted);
                };
                AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

                FOnMontageEnded MontageEndedDelegate = [this](UAnimMontage* Montage, bool bInterrupted) {
                    OnMontageEnded(Montage, bInterrupted);
                };
                AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

                FScriptDelegate NotifyBegin;
                NotifyBegin.BindUFunction(this, FName("OnNotifyBeginReceived"));
                AnimInstance->OnPlayMontageNotifyBegin.AddUnique(NotifyBegin);
                FScriptDelegate NotifyEnd;
                NotifyEnd.BindUFunction(this, FName("OnNotifyEndReceived"));
                AnimInstance->OnPlayMontageNotifyEnd.AddUnique(NotifyEnd);
            }
        }
    }

    if (!bPlayedSuccessfully) BroadcastName(OnInterrupted, FName());
    return bPlayedSuccessfully;
}

bool UPlayMontageCallbackProxy::IsNotifyValid(FName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const {
    return MontageInstanceID != INDEX_NONE && BranchingPointNotifyPayload.MontageInstanceID == MontageInstanceID;
}

void UPlayMontageCallbackProxy::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) {
    if (IsNotifyValid(NotifyName, BranchingPointNotifyPayload)) BroadcastName(OnNotifyBegin, NotifyName);
}

void UPlayMontageCallbackProxy::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) {
    if (IsNotifyValid(NotifyName, BranchingPointNotifyPayload)) BroadcastName(OnNotifyEnd, NotifyName);
}

void UPlayMontageCallbackProxy::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) {
    if (bInterrupted) {
        BroadcastName(OnInterrupted, FName());
        bInterruptedCalledBeforeBlendingOut = true;
    } else {
        BroadcastName(OnBlendOut, FName());
    }
}

void UPlayMontageCallbackProxy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) {
    if (!bInterrupted) {
        BroadcastName(OnCompleted, FName());
    } else if (!bInterruptedCalledBeforeBlendingOut) {
        BroadcastName(OnInterrupted, FName());
    }
    UnbindDelegates();
}

void UPlayMontageCallbackProxy::UnbindDelegates() {
    if (AnimInstancePtr == nullptr) return;
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : AnimInstancePtr->MontageInstances) {
        if (MontageInstance && MontageInstance->GetInstanceID() == MontageInstanceID) {
            MontageInstance->OnMontageBlendingOutStarted = nullptr;
            MontageInstance->OnMontageEnded = nullptr;
            break;
        }
    }
    AnimInstancePtr->OnPlayMontageNotifyBegin.Remove(this, FName("OnNotifyBeginReceived"));
    AnimInstancePtr->OnPlayMontageNotifyEnd.Remove(this, FName("OnNotifyEndReceived"));
}

void UPlayMontageCallbackProxy::BeginDestroy() {
    UnbindDelegates();
    UObject::BeginDestroy();
}

}
