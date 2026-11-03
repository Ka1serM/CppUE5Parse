#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_PlayParticleEffect.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;
class UParticleSystemComponent;

// UAnimNotify_PlayParticleEffect (UE's Animation/AnimNotifies/AnimNotify_PlayParticleEffect.h): spawns a particle
// system at a socket of the mesh, attached or in world space.
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
