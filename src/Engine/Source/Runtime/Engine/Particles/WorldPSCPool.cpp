#include "Engine/Source/Runtime/Engine/Particles/WorldPSCPool.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

namespace {

//! The console variables FX.ParticleSystemPool.KillUnusedTime and FX.ParticleSystemPool.CleanTime at their defaults.
constexpr float ParticleSystemPoolKillUnusedTime = 180;
constexpr float ParticleSystemPoolingCleanTime = 30;

}

UParticleSystemComponent* FPSCPool::Acquire(UWorld* World, UParticleSystem* Template, const EPSCPoolMethod PoolingMethod) {
    UParticleSystemComponent* PSC = nullptr;
    if (!FreeElements.IsEmpty()) {
        PSC = FreeElements.Pop().PSC;
        PSC->SetVisibility(true, false);
        PSC->SetHiddenInGame(false, false);
    } else {
        AActor* OuterActor = World->GetWorldSettings();
        UObject* OuterObject = OuterActor != nullptr ? static_cast<UObject*>(OuterActor) : static_cast<UObject*>(World);
        PSC = NewObject<UParticleSystemComponent>(OuterObject);
        PSC->bAutoDestroy = false;
        PSC->SecondsBeforeInactive = 0;
        PSC->bAutoActivate = false;
        PSC->SetTemplate(Template);
        PSC->bOverrideLODMethod = false;
        PSC->bAllowRecycling = true;
    }
    PSC->PoolingMethod = PoolingMethod;
    return PSC;
}

void FPSCPool::Reclaim(UParticleSystemComponent* PSC, const float CurrentTimeSeconds) {
    if (FreeElements.Num() >= static_cast<int32>(PSC->Template->MaxPoolSize)) {
        PSC->PoolingMethod = EPSCPoolMethod::None;
        PSC->DestroyComponent();
        return;
    }
    PSC->bHasBeenActivated = false;
    PSC->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    PSC->SetRelativeScale3D(FVector(1));
    PSC->SetAbsolute();
    PSC->UnregisterComponent();
    PSC->SetCastShadow(false);
    PSC->OnParticleSpawn.Clear();
    PSC->OnParticleBurst.Clear();
    PSC->OnParticleDeath.Clear();
    PSC->OnParticleCollide.Clear();
    PSC->bWasActive = false;
    PSC->InstanceParameters.Reset();
    PSC->PoolingMethod = EPSCPoolMethod::FreeInPool;
    FreeElements.Push(FPSCPoolElem{.PSC = PSC, .LastUsedTime = CurrentTimeSeconds});
}

void FPSCPool::KillUnusedComponents(const float KillTime) {
    int32 Index = 0;
    while (Index < FreeElements.Num()) {
        if (FreeElements[Index].LastUsedTime >= KillTime) {
            ++Index;
            continue;
        }
        UParticleSystemComponent* PSC = FreeElements[Index].PSC;
        PSC->PoolingMethod = EPSCPoolMethod::None;
        PSC->DestroyComponent();
        FreeElements.RemoveAtSwap(Index);
    }
}

UParticleSystemComponent* FWorldPSCPool::CreateWorldParticleSystem(UParticleSystem* Template, UWorld* World, const EPSCPoolMethod PoolingMethod) {
    if (Template == nullptr)
        return nullptr;
    if (Template->CanBePooled())
        return WorldParticleSystemPools.FindOrAdd(Template).Acquire(World, Template, PoolingMethod);
    UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(World);
    PSC->bAutoDestroy = true;
    PSC->SecondsBeforeInactive = 0;
    PSC->bAutoActivate = false;
    PSC->SetTemplate(Template);
    PSC->bOverrideLODMethod = false;
    return PSC;
}

void FWorldPSCPool::ReclaimWorldParticleSystem(UParticleSystemComponent* PSC) {
    if (!IsValid(PSC))
        return;
    const float CurrentTime = static_cast<float>(PSC->GetWorld()->GetTimeSeconds());
    if (CurrentTime - LastParticleSytemPoolCleanTime > ParticleSystemPoolingCleanTime) {
        LastParticleSytemPoolCleanTime = CurrentTime;
        for (auto& [Template, Pool] : WorldParticleSystemPools)
            Pool.KillUnusedComponents(CurrentTime - ParticleSystemPoolKillUnusedTime);
    }
    WorldParticleSystemPools.FindOrAdd(PSC->Template).Reclaim(PSC, CurrentTime);
}

}
