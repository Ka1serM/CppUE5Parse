#pragma once

#include "Engine/Source/Runtime/Engine/Distributions/Distributions.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleHelper.h"

#include "ParticleModule.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;
class UParticleLODLevel;
class UMaterialInstanceDynamic;
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

USTRUCT()
struct FParticleRandomSeedInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    bool bGetSeedFromInstance = false;
    UPROPERTY()
    bool bInstanceSeedIsIndex = false;
    UPROPERTY()
    bool bResetSeedOnEmitterLooping = true;
    UPROPERTY()
    bool bRandomlySelectSeedArray = false;
    UPROPERTY()
    TArray<int32> RandomSeeds;
};

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
    UPROPERTY()
    uint8 LODValidity = 0;

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {}
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {}
    virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {}
    virtual uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) { return 0; }
    virtual uint32 RequiredBytesPerInstance() { return 0; }
    virtual uint32 PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) { return 0xffffffff; }
    virtual EModuleType GetModuleType() const { return EPMT_General; }
    virtual bool IsSizeMultiplyLife() { return false; }
    bool RequiresLoopingNotification() const { return bRequiresLoopingNotification; }
    virtual void EmitterLoopingNotify(FParticleEmitterInstance* Owner) {}
    virtual bool TouchesMeshRotation() const { return false; }
    virtual bool IsCustomColor() const { return false; }
    virtual void ApplyCustomColor(UParticleSystemComponent* Component, UMaterialInstanceDynamic* MID) {}
    uint32 PrepRandomSeedInstancePayload(FParticleEmitterInstance* Owner, FParticleRandomSeedInstancePayload* InRandSeedPayload,
                                         const FParticleRandomSeedInfo& InRandSeedInfo);
    virtual FParticleRandomSeedInfo* GetRandomSeedInfo() { return nullptr; }
    FRandomStream& GetRandomStream(FParticleEmitterInstance* Owner);
};

}
