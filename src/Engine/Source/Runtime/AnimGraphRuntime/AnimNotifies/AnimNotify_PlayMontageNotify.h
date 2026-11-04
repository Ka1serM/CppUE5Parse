#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotify_PlayMontageNotify.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimNotify_PlayMontageNotify : public UAnimNotify {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName NotifyName;

    explicit UAnimNotify_PlayMontageNotify(const FObjectInitializer& ObjectInitializer);
    void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};

UCLASS()
class UAnimNotify_PlayMontageNotifyWindow : public UAnimNotifyState {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName NotifyName;

    explicit UAnimNotify_PlayMontageNotifyWindow(const FObjectInitializer& ObjectInitializer);
    void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;
    void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};

}
