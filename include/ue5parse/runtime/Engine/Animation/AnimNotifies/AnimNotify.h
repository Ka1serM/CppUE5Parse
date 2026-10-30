#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"

#include "AnimNotify.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;

// A branching point of a montage, which fires during the montage's advance rather than with the queued notifies.
USTRUCT()
struct FBranchingPointNotifyPayload {
    GENERATED_BODY()
    USkeletalMeshComponent* SkelMeshComponent = nullptr;
    UAnimSequenceBase* SequenceAsset = nullptr;
    FAnimNotifyEvent* NotifyEvent = nullptr;
    int32 MontageInstanceID = INDEX_NONE;
    bool bReachedEnd = false;

    FBranchingPointNotifyPayload() = default;
    FBranchingPointNotifyPayload(USkeletalMeshComponent* InSkelMeshComponent, UAnimSequenceBase* InSequenceAsset, FAnimNotifyEvent* InNotifyEvent,
                                 int32 InMontageInstanceID, bool bInReachedEnd = false)
        : SkelMeshComponent(InSkelMeshComponent), SequenceAsset(InSequenceAsset), NotifyEvent(InNotifyEvent), MontageInstanceID(InMontageInstanceID),
          bReachedEnd(bInReachedEnd) {}
};

// UAnimNotify (UE's Animation/AnimNotifies/AnimNotify.h): an event at one time of an animation. A Blueprint notify
// implements Received_Notify; a native one overrides Notify.
UCLASS(Abstract)
class UAnimNotify : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    bool Received_Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {}
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload);
    // The world of the mesh the notify is firing for.
    UWorld* GetWorld() const override;

    bool bIsNativeBranchingPoint = false;

private:
    USkeletalMeshComponent* MeshContext = nullptr;
};

}
