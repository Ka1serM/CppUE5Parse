#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotifyState.generated.h"

namespace ue5parse::runtime {

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
