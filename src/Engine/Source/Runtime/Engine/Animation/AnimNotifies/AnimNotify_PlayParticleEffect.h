#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_PlayParticleEffect.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;
class UParticleSystemComponent;

UCLASS(const)
class UAnimNotify_PlayParticleEffect : public UAnimNotify {
    GENERATED_BODY()
public:
    explicit UAnimNotify_PlayParticleEffect(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UParticleSystem> PSTemplate = nullptr;
    UPROPERTY()
    FVector LocationOffset = FVector::ZeroVector;
    UPROPERTY()
    FRotator RotationOffset = FRotator::ZeroRotator;
    UPROPERTY()
    FVector Scale = FVector::ZeroVector;
    UPROPERTY()
    bool Attached = false;
    UPROPERTY()
    FName SocketName;

    using UAnimNotify::Notify;
    void PostLoad() override;
    void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
    virtual UParticleSystemComponent* SpawnParticleSystem(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);

private:
    FQuat RotationOffsetQuat = FQuat::Identity;
};

}
