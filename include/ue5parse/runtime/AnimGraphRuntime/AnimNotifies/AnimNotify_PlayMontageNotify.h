#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotify_PlayMontageNotify.generated.h"

namespace ue5parse::runtime {

// A montage notify that a Play Montage node receives as OnNotifyBegin (UE's
// AnimNotifies/AnimNotify_PlayMontageNotify.h). It fires as a branching point, with the montage's advance.
UCLASS()
class UAnimNotify_PlayMontageNotify : public UAnimNotify {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName NotifyName;

    explicit UAnimNotify_PlayMontageNotify(const FObjectInitializer& ObjectInitializer);
    void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};

// A montage notify window: OnNotifyBegin at its start and OnNotifyEnd at its end.
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
