#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_TimedParticleEffect.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;

// UAnimNotifyState_TimedParticleEffect (UE's Animation/AnimNotifies/AnimNotifyState_TimedParticleEffect.h): a
// particle system attached to a socket of the mesh for the state's time range. At the end it is destroyed or
// deactivated.
UCLASS()
class UAnimNotifyState_TimedParticleEffect : public UAnimNotifyState {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UParticleSystem> PSTemplate = nullptr;
    UPROPERTY()
    FName SocketName;
    UPROPERTY()
    FVector LocationOffset = FVector::ZeroVector;
    UPROPERTY()
    FRotator RotationOffset = FRotator::ZeroRotator;
    UPROPERTY()
    bool bDestroyAtEnd = false;

    using UAnimNotifyState::NotifyBegin;
    using UAnimNotifyState::NotifyEnd;
    using UAnimNotifyState::NotifyTick;
    void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                     const FAnimNotifyEventReference& EventReference) override;
    void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                    const FAnimNotifyEventReference& EventReference) override;
    void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
    bool ValidateParameters(USkeletalMeshComponent* MeshComp);
};

}
