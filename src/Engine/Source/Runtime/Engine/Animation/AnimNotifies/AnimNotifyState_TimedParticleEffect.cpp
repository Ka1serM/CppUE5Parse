#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotifyState_TimedParticleEffect.h"

#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Kismet/GameplayStatics.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

void UAnimNotifyState_TimedParticleEffect::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                                       const FAnimNotifyEventReference& EventReference) {
    NotifyBegin(MeshComp, Animation, TotalDuration);
    if (ValidateParameters(MeshComp))
        UGameplayStatics::SpawnEmitterAttached(PSTemplate, MeshComp, SocketName, LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset,
                                               !bDestroyAtEnd);
    Received_NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UAnimNotifyState_TimedParticleEffect::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                                                      const FAnimNotifyEventReference& EventReference) {
    NotifyTick(MeshComp, Animation, FrameDeltaTime);
    Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UAnimNotifyState_TimedParticleEffect::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                     const FAnimNotifyEventReference& EventReference) {
    NotifyEnd(MeshComp, Animation);
    TArray<USceneComponent*> Children;
    MeshComp->GetChildrenComponents(false, Children);
    for (USceneComponent* Component : Children) {
        UParticleSystemComponent* ParticleComponent = Cast<UParticleSystemComponent>(Component);
        if (ParticleComponent == nullptr)
            continue;
        const bool bSocketMatch = ParticleComponent->GetAttachSocketName() == SocketName;
        const bool bTemplateMatch = ParticleComponent->Template == PSTemplate;
        if (bSocketMatch && bTemplateMatch && !ParticleComponent->bWasDeactivated) {
            if (bDestroyAtEnd)
                ParticleComponent->DestroyComponent();
            else
                ParticleComponent->DeactivateSystem();
            break;
        }
    }
    Received_NotifyEnd(MeshComp, Animation, EventReference);
}

bool UAnimNotifyState_TimedParticleEffect::ValidateParameters(USkeletalMeshComponent* MeshComp) {
    if (PSTemplate == nullptr)
        return false;
    return MeshComp->DoesSocketExist(SocketName) || MeshComp->GetBoneIndex(SocketName) != INDEX_NONE;
}

}
