#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotifyState.generated.h"

namespace ue5parse::runtime {

// UAnimNotifyState (UE's Animation/AnimNotifies/AnimNotifyState.h): an event over a time range of an animation: it
// begins, ticks each frame and ends. A Blueprint state implements the Received_ events; a native one overrides the
// Notify functions.
UCLASS(Abstract)
class UAnimNotifyState : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    bool Received_NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                              const FAnimNotifyEventReference& EventReference) const;
    UFUNCTION(BlueprintImplementableEvent)
    bool Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                             const FAnimNotifyEventReference& EventReference) const;
    UFUNCTION(BlueprintImplementableEvent)
    bool Received_NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) {}
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) {}
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {}
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                             const FAnimNotifyEventReference& EventReference);
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                            const FAnimNotifyEventReference& EventReference);
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload);
    virtual void BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime);
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload);

    bool bIsNativeBranchingPoint = false;
};

}
