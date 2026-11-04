#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_TimedParticleEffect.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;

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
