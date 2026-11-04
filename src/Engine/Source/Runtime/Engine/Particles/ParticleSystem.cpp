#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleCollision.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSpawn.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"

namespace ue5parse::runtime {

void UParticleSystem::PostLoad() {
    Super::PostLoad();
    bHasPhysics = false;
    bAnyEmitterLoopsForever = false;
    bIsImmortal = false;
    bWillBecomeZombie = false;
    HighestSignificance = EParticleSignificanceLevel::Low;
    LowestSignificance = EParticleSignificanceLevel::Critical;

    for (int32 i = Emitters.Num() - 1; i >= 0; i--) {
        UParticleEmitter* Emitter = Emitters[i];
        if (Emitter == nullptr)
            continue;
        Emitter->ConditionalPostLoad();
        const UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(Emitter);
        if (SpriteEmitter && SpriteEmitter->bCookedOut)
            continue;
        if (Emitter->LODLevels.Num() == 0)
            continue;
        UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
        LODLevel->ConditionalPostLoad();
        for (UParticleLODLevel* ParticleLODLevel : Emitter->LODLevels) {
            if (!ParticleLODLevel)
                continue;
            for (int32 ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++) {
                const UParticleModuleCollision* CollisionModule = Cast<UParticleModuleCollision>(ParticleLODLevel->Modules[ModuleIndex]);
                if (CollisionModule && CollisionModule->bApplyPhysics) {
                    bHasPhysics = true;
                    break;
                }
            }
            if (!LODLevel->bEnabled)
                continue;
            const UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
            if (RequiredModule == nullptr || RequiredModule->EmitterLoops != 0)
                continue;
            bAnyEmitterLoopsForever = true;
            if (RequiredModule->EmitterDuration == 0.0f) {
                bIsImmortal = true;
                if (LODLevel->SpawnModule->GetMaximumSpawnRate() == 0.0f && !bAutoDeactivate)
                    bWillBecomeZombie = true;
            }
        }
        const EParticleSignificanceLevel EmitterSignificance = FMath::Min(MaxSignificanceLevel, Emitter->SignificanceLevel);
        if (EmitterSignificance > HighestSignificance)
            HighestSignificance = EmitterSignificance;
        if (EmitterSignificance < LowestSignificance)
            LowestSignificance = EmitterSignificance;
    }
    bShouldManageSignificance = GetLowestSignificance() != EParticleSignificanceLevel::Critical;

    if (LODSettings.Num() == 0) {
        if (Emitters.Num() == 0)
            LODSettings.SetNum(1);
        else if (Emitters[0])
            LODSettings.SetNum(Emitters[0]->LODLevels.Num());
    }
    if (LODDistances.Num() == 0 && Emitters.Num() > 0 && Emitters[0]) {
        LODDistances.SetNum(Emitters[0]->LODLevels.Num());
        for (int32 LODIndex = 0; LODIndex < LODDistances.Num(); LODIndex++)
            LODDistances[LODIndex] = LODIndex * 2500.0f;
    }
    FixedRelativeBoundingBox.IsValid = true;
}

}
