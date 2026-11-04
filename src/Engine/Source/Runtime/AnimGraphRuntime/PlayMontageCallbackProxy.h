#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "PlayMontageCallbackProxy.generated.h"

namespace ue5parse::runtime {

class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMontagePlayDelegate, FName, NotifyName);

UCLASS()
class UPlayMontageCallbackProxy : public UObject {
    GENERATED_BODY()
public:
    explicit UPlayMontageCallbackProxy(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintAssignable)
    FMulticastScriptDelegate OnCompleted;
    UPROPERTY(BlueprintAssignable)
    FMulticastScriptDelegate OnBlendOut;
    UPROPERTY(BlueprintAssignable)
    FMulticastScriptDelegate OnInterrupted;
    UPROPERTY(BlueprintAssignable)
    FMulticastScriptDelegate OnNotifyBegin;
    UPROPERTY(BlueprintAssignable)
    FMulticastScriptDelegate OnNotifyEnd;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
    static UPlayMontageCallbackProxy* CreateProxyObjectForPlayMontage(USkeletalMeshComponent* InSkeletalMeshComponent, UAnimMontage* MontageToPlay,
                                                                      float PlayRate = 1.0f, float StartingPosition = 0.0f,
                                                                      FName StartingSection = FName());

    bool PlayMontage(USkeletalMeshComponent* InSkeletalMeshComponent, UAnimMontage* MontageToPlay, float PlayRate, float StartingPosition,
                     FName StartingSection);

    void BeginDestroy() override;

protected:
    void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    UFUNCTION()
    void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);
    UFUNCTION()
    void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);
    bool IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const;
    void UnbindDelegates();

private:
    UAnimInstance* AnimInstancePtr = nullptr;
    int32 MontageInstanceID = INDEX_NONE;
    bool bInterruptedCalledBeforeBlendingOut = false;
};

}
