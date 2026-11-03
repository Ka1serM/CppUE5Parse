#pragma once

#include "ue5parse/runtime/Engine/Distributions/Distributions.h"
#include "ue5parse/runtime/Engine/Particles/ParticleHelper.h"

#include "ParticleModule.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;
class UParticleLODLevel;
class UParticleModuleTypeDataBase;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

UENUM()
enum EModuleType : int32 {
    EPMT_General,
    EPMT_TypeData,
    EPMT_Beam,
    EPMT_Trail,
    EPMT_Spawn,
    EPMT_Required,
    EPMT_Event,
    EPMT_Light,
    EPMT_SubUV,
    EPMT_MAX,
};

UENUM()
enum EParticleSourceSelectionMethod : int32 { EPSSM_Random, EPSSM_Sequential, EPSSM_MAX };

// FParticleRandomSeedInfo: how a seeded module seeds its own random stream (UE's Particles/ParticleModule.h).
USTRUCT()
struct FParticleRandomSeedInfo {
    GENERATED_BODY()
    // The float instance parameter that gives the seed, when bGetSeedFromInstance.
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    bool bGetSeedFromInstance = false;
    // The parameter is an index into RandomSeeds, not the seed.
    UPROPERTY()
    bool bInstanceSeedIsIndex = false;
    UPROPERTY()
    bool bResetSeedOnEmitterLooping = true;
    UPROPERTY()
    bool bRandomlySelectSeedArray = false;
    UPROPERTY()
    TArray<int32> RandomSeeds;

    int32 GetInstancePayloadSize() const { return RandomSeeds.Num() > 0 ? int32(sizeof(FParticleRandomSeedInstancePayload)) : 0; }
};

// UParticleModule: one behavior of a Cascade emitter LOD level (UE's Particles/ParticleModule.h). A module spawns
// particles (initial values), updates them every frame, or both. Its per-particle data lives at Offset in each
// particle, its per-instance data in the instance data of the emitter instance.
UCLASS(abstract)
class UParticleModule : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bSpawnModule = false;
    UPROPERTY()
    bool bUpdateModule = false;
    UPROPERTY()
    bool bFinalUpdateModule = false;
    UPROPERTY()
    bool bUpdateForGPUEmitter = false;
    UPROPERTY()
    bool bCurvesAsColor = false;
    UPROPERTY()
    bool b3DDrawMode = false;
    UPROPERTY()
    bool bSupported3DDrawMode = false;
    UPROPERTY()
    bool bEnabled = true;
    UPROPERTY()
    bool bEditable = true;
    UPROPERTY()
    bool LODDuplicate = true;
    UPROPERTY()
    bool bSupportsRandomSeed = false;
    UPROPERTY()
    bool bRequiresLoopingNotification = false;
    // A bit per LOD level that uses this module.
    UPROPERTY()
    uint8 LODValidity = 0;

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {}
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {}
    virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {}
    // The bytes this module adds to each particle.
    virtual uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) { return 0; }
    // The bytes this module adds to the instance data of the emitter instance.
    virtual uint32 RequiredBytesPerInstance() { return 0; }
    // Fills this module's instance data; 0xffffffff when it has none to fill.
    virtual uint32 PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) { return 0xffffffff; }
    virtual EModuleType GetModuleType() const { return EPMT_General; }
    virtual bool IsSizeMultiplyLife() { return false; }
    bool SupportsRandomSeed() const { return bSupportsRandomSeed; }
    bool RequiresLoopingNotification() const { return bRequiresLoopingNotification; }
    virtual void EmitterLoopingNotify(FParticleEmitterInstance* Owner) {}
    bool IsUsedInLODLevel(int32 SourceLODIndex) const { return SourceLODIndex >= 0 && SourceLODIndex < 8 && (LODValidity & (1 << SourceLODIndex)) != 0; }
    virtual bool TouchesMeshRotation() const { return false; }
    // Seeds the random stream of a seeded module; 0xffffffff when the module draws from the component's stream.
    uint32 PrepRandomSeedInstancePayload(FParticleEmitterInstance* Owner, FParticleRandomSeedInstancePayload* InRandSeedPayload,
                                         const FParticleRandomSeedInfo& InRandSeedInfo);
    virtual FParticleRandomSeedInfo* GetRandomSeedInfo() { return nullptr; }
    // This module's seeded stream, else the component's.
    FRandomStream& GetRandomStream(FParticleEmitterInstance* Owner);
};

}
