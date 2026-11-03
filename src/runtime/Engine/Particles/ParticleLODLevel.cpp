#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"

#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleEvent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleLifetime.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleOrbit.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleSpawn.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/StaticMeshResources.h"

// UE's UParticleLODLevel (ParticleComponents.cpp, UE 5.3).

namespace ue5parse::runtime {

void UParticleLODLevel::PostLoad() {
    Super::PostLoad();
    RequiredModule->ConditionalPostLoad();
    if (SpawnModule)
        SpawnModule->ConditionalPostLoad();
    for (UParticleModule* ParticleModule : Modules)
        ParticleModule->ConditionalPostLoad();
}

void UParticleLODLevel::UpdateModuleLists() {
    SpawningModules.Empty();
    SpawnModules.Empty();
    UpdateModules.Empty();
    OrbitModules.Empty();
    EventReceiverModules.Empty();
    EventGenerator = nullptr;

    int32 TypeDataModuleIndex = -1;
    for (int32 i = 0; i < Modules.Num(); i++) {
        UParticleModule* Module = Modules[i];
        if (!Module)
            continue;
        if (Module->bSpawnModule)
            SpawnModules.Add(Module);
        if (Module->bUpdateModule || Module->bFinalUpdateModule)
            UpdateModules.Add(Module);
        if (UParticleModuleTypeDataBase* TypeData = Cast<UParticleModuleTypeDataBase>(Module)) {
            TypeDataModule = TypeData;
            if (!Module->bSpawnModule && !Module->bUpdateModule)
                TypeDataModuleIndex = i;
        } else if (UParticleModuleSpawnBase* SpawnBase = Cast<UParticleModuleSpawnBase>(Module)) {
            SpawningModules.Add(SpawnBase);
        } else if (UParticleModuleOrbit* Orbit = Cast<UParticleModuleOrbit>(Module)) {
            OrbitModules.Add(Orbit);
        } else if (UParticleModuleEventGenerator* Generator = Cast<UParticleModuleEventGenerator>(Module)) {
            EventGenerator = Generator;
        } else if (UParticleModuleEventReceiverBase* Receiver = Cast<UParticleModuleEventReceiverBase>(Module)) {
            EventReceiverModules.Add(Receiver);
        }
    }

    if (EventGenerator) {
        const int32 GeneratorIndex = Modules.Find(EventGenerator.Get());
        Modules.RemoveAt(GeneratorIndex);
        Modules.Insert(EventGenerator.Get(), 0);
    }
    // UE removes by the index found before the generator moved, as here.
    if (TypeDataModuleIndex != -1)
        Modules.RemoveAt(TypeDataModuleIndex);

    if (TypeDataModule) {
        UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(TypeDataModule.Get());
        UStaticMesh* Mesh = MeshTD ? MeshTD->Mesh.Get() : nullptr;
        FStaticMeshRenderData* RenderData = Mesh ? Mesh->GetRenderData() : nullptr;
        if (RenderData && RenderData->LODResources.Num() > 0 && RenderData->LODResources[0].Sections.Num() > 0) {
            UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(GetOuter());
            if (SpriteEmitter && !MeshTD->bOverrideMaterial) {
                const FStaticMeshSection& Section = RenderData->LODResources[0].Sections[0];
                if (UMaterialInterface* Material = Mesh->GetMaterial(Section.MaterialIndex))
                    RequiredModule->Material = Material;
            }
        }
    }
}

int32 UParticleLODLevel::CalculateMaxActiveParticleCount() {
    float ParticleLifetime = 0.0f;
    float MaxSpawnRate = SpawnModule->GetEstimatedSpawnRate();
    int32 MaxBurstCount = SpawnModule->GetMaximumBurstCount();
    for (UParticleModule* Module : Modules) {
        if (UParticleModuleLifetimeBase* LifetimeMod = Cast<UParticleModuleLifetimeBase>(Module))
            ParticleLifetime += LifetimeMod->GetMaxLifetime();
        if (UParticleModuleSpawnBase* SpawnMod = Cast<UParticleModuleSpawnBase>(Module)) {
            MaxSpawnRate += SpawnMod->GetEstimatedSpawnRate();
            MaxBurstCount += SpawnMod->GetMaximumBurstCount();
        }
    }

    float MaxDuration = 0.0f;
    float TotalDuration = 0.0f;
    int32 TotalLoops = 0;
    if (RequiredModule) {
        MaxDuration = FMath::Max<float>(RequiredModule->EmitterDuration, RequiredModule->EmitterDurationLow);
        TotalLoops = RequiredModule->EmitterLoops;
        TotalDuration = MaxDuration * TotalLoops;
    }

    // The int32 += float statements truncate as UE's do.
    int32 MaxAPC = 0;
    if (TotalDuration != 0.0f) {
        if (TotalLoops == 1) {
            if (ParticleLifetime < MaxDuration)
                MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
            else
                MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
            MaxAPC += 1;
            MaxAPC += MaxBurstCount;
        } else {
            if (ParticleLifetime < MaxDuration)
                MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
            else
                MaxAPC += FMath::CeilToInt(FMath::CeilToInt(MaxDuration * MaxSpawnRate) * ParticleLifetime);
            MaxAPC += 1;
            MaxAPC += MaxBurstCount;
            if (ParticleLifetime > MaxDuration)
                MaxAPC += MaxBurstCount * FMath::CeilToInt(ParticleLifetime - MaxDuration);
        }
    } else {
        if (ParticleLifetime < MaxDuration) {
            MaxAPC += FMath::CeilToInt(ParticleLifetime * FMath::CeilToInt(MaxSpawnRate));
        } else if (ParticleLifetime != 0.0f) {
            if (ParticleLifetime <= MaxDuration)
                MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
            else
                MaxAPC = int32(float(MaxAPC) + float(FMath::CeilToInt(MaxDuration * MaxSpawnRate)) * ParticleLifetime);
        } else {
            MaxAPC += FMath::CeilToInt(MaxSpawnRate);
        }
        MaxAPC += FMath::Max<int32>(FMath::CeilToInt(MaxSpawnRate * 0.032f), 2);
        MaxAPC += MaxBurstCount;
    }

    PeakActiveParticles = MaxAPC;
    return MaxAPC;
}

int32 UParticleLODLevel::GetModuleIndex(UParticleModule* InModule) const {
    if (InModule == nullptr)
        return INDEX_NONE;
    if (InModule == RequiredModule.Get())
        return INDEX_REQUIREDMODULE;
    if (InModule == SpawnModule.Get())
        return INDEX_SPAWNMODULE;
    if (InModule == TypeDataModule.Get())
        return INDEX_TYPEDATAMODULE;
    for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
        if (InModule == Modules[ModuleIndex].Get())
            return ModuleIndex;
    return INDEX_NONE;
}

UParticleModule* UParticleLODLevel::GetModuleAtIndex(int32 InIndex) const {
    if (InIndex > INDEX_NONE)
        return InIndex < Modules.Num() ? Modules[InIndex].Get() : nullptr;
    switch (InIndex) {
    case INDEX_REQUIREDMODULE:
        return RequiredModule;
    case INDEX_SPAWNMODULE:
        return SpawnModule;
    case INDEX_TYPEDATAMODULE:
        return TypeDataModule;
    }
    return nullptr;
}

}
