#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify_PlayParticleEffect.h"

#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Kismet/GameplayStatics.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"

namespace ue5parse::runtime {

UAnimNotify_PlayParticleEffect::UAnimNotify_PlayParticleEffect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Attached = true;
    Scale = FVector(1.f);
}

void UAnimNotify_PlayParticleEffect::PostLoad() {
    Super::PostLoad();
    RotationOffsetQuat = FQuat(RotationOffset);
}

void UAnimNotify_PlayParticleEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) {
    Notify(MeshComp, Animation);
    SpawnParticleSystem(MeshComp, Animation);
}

//! A looping system is not spawned: nothing would stop it.
UParticleSystemComponent* UAnimNotify_PlayParticleEffect::SpawnParticleSystem(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {
    UParticleSystemComponent* ReturnComp = nullptr;
    if (PSTemplate == nullptr || PSTemplate->IsLooping())
        return ReturnComp;
    if (Attached)
        ReturnComp = UGameplayStatics::SpawnEmitterAttached(PSTemplate, MeshComp, SocketName, LocationOffset, RotationOffset, Scale);
    else {
        const FTransform MeshTransform = MeshComp->GetSocketTransform(SocketName);
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(MeshTransform.TransformPosition(LocationOffset));
        SpawnTransform.SetRotation(MeshTransform.GetRotation() * RotationOffsetQuat);
        SpawnTransform.SetScale3D(Scale);
        ReturnComp = UGameplayStatics::SpawnEmitterAtLocation(MeshComp->GetWorld(), PSTemplate, SpawnTransform);
    }
    return ReturnComp;
}

}
