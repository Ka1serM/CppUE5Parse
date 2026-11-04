#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifyQueue.h"

#include "AnimNotify.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;

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

UCLASS(Abstract)
class UAnimNotify : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    bool Received_Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {}
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload);
    UWorld* GetWorld() const override;

    bool bIsNativeBranchingPoint = false;

private:
    USkeletalMeshComponent* MeshContext = nullptr;
};

}
