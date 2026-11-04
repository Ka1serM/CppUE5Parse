#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"

#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleEvent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLifetime.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleOrbit.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSpawn.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

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
