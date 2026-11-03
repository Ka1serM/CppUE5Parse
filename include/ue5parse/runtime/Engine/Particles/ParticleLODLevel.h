#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleLODLevel.generated.h"

namespace ue5parse::runtime {

class UParticleModuleEventGenerator;
class UParticleModuleEventReceiverBase;
class UParticleModuleOrbit;
class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleSpawnBase;
class UParticleModuleTypeDataBase;

// UParticleLODLevel: the modules of one LOD of an emitter (UE's Particles/ParticleLODLevel.h). The module lists by
// role are transient: UpdateModuleLists builds them after load.
UCLASS()
class UParticleLODLevel : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 Level = 0;
    UPROPERTY()
    bool bEnabled = true;
    UPROPERTY()
    TObjectPtr<UParticleModuleRequired> RequiredModule = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModule>> Modules;
    UPROPERTY()
    TObjectPtr<UParticleModule> ModulePendingDelete = nullptr;
    UPROPERTY()
    TObjectPtr<UParticleModuleTypeDataBase> TypeDataModule = nullptr;
    UPROPERTY()
    TObjectPtr<UParticleModuleSpawn> SpawnModule = nullptr;
    UPROPERTY()
    TObjectPtr<UParticleModuleEventGenerator> EventGenerator = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModuleSpawnBase>> SpawningModules;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModule>> SpawnModules;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModule>> UpdateModules;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModuleOrbit>> OrbitModules;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModuleEventReceiverBase>> EventReceiverModules;
    UPROPERTY()
    bool ConvertedModules = true;
    UPROPERTY()
    int32 PeakActiveParticles = 0;

    void PostLoad() override;
    virtual void UpdateModuleLists();
    virtual int32 CalculateMaxActiveParticleCount();
    int32 GetModuleIndex(UParticleModule* InModule) const;
    UParticleModule* GetModuleAtIndex(int32 InIndex) const;
};

}
