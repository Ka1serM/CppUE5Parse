#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleEmitter.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UParticleLODLevel;
class UParticleSystemComponent;
class USubUVAnimation;
struct FParticleEmitterInstance;

UENUM()
enum EParticleBurstMethod : int32 { EPBM_Instant, EPBM_Interpolated, EPBM_MAX };

UENUM()
enum EParticleSubUVInterpMethod : int32 { PSUVIM_None, PSUVIM_Linear, PSUVIM_Linear_Blend, PSUVIM_Random, PSUVIM_Random_Blend, PSUVIM_MAX };

UENUM()
enum EEmitterRenderMode : int32 { ERM_Normal, ERM_Point, ERM_Cross, ERM_LightsOnly, ERM_None, ERM_MAX };

UENUM()
enum class EParticleSignificanceLevel : uint8 { Low, Medium, High, Critical, Num };

UENUM()
enum EParticleScreenAlignment : int32 {
    PSA_FacingCameraPosition,
    PSA_Square,
    PSA_Rectangle,
    PSA_Velocity,
    PSA_AwayFromCenter,
    PSA_TypeSpecific,
    PSA_FacingCameraDistanceBlend,
    PSA_MAX,
};

UENUM()
enum EParticleAxisLock : int32 {
    EPAL_NONE,
    EPAL_X,
    EPAL_Y,
    EPAL_Z,
    EPAL_NEGATIVE_X,
    EPAL_NEGATIVE_Y,
    EPAL_NEGATIVE_Z,
    EPAL_ROTATE_X,
    EPAL_ROTATE_Y,
    EPAL_ROTATE_Z,
    EPAL_MAX,
};

USTRUCT()
struct FParticleBurst {
    GENERATED_BODY()
    UPROPERTY()
    int32 Count = 0;
    UPROPERTY()
    int32 CountLow = -1;
    UPROPERTY()
    float Time = 0;
};

UCLASS(abstract)
class UParticleEmitter : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName EmitterName = FName("Particle Emitter");
    UPROPERTY()
    int32 SubUVDataOffset = 0;
    UPROPERTY()
    TEnumAsByte<EEmitterRenderMode> EmitterRenderMode = ERM_Normal;
    UPROPERTY()
    EParticleSignificanceLevel SignificanceLevel = EParticleSignificanceLevel::Critical;
    UPROPERTY()
    bool bUseLegacySpawningBehavior = false;
    UPROPERTY()
    bool ConvertedModules = true;
    UPROPERTY()
    bool bIsSoloing = false;
    UPROPERTY()
    bool bCookedOut = false;
    UPROPERTY()
    bool bDisabledLODsKeepEmitterAlive = false;
    UPROPERTY()
    bool bDisableWhenInsignficant = false;
    UPROPERTY()
    TArray<TObjectPtr<UParticleLODLevel>> LODLevels;
    UPROPERTY()
    int32 PeakActiveParticles = 0;
    UPROPERTY()
    int32 InitialAllocationCount = 0;
    UPROPERTY()
    float QualityLevelSpawnRateScale = 1.0f;
    UPROPERTY()
    uint32 DetailModeBitmask = PDM_DefaultValue;

    TEnumAsByte<EParticleAxisLock> LockAxisFlags = EPAL_NONE;
    bool bRequiresLoopNotification = false;
    bool bAxisLockEnabled = false;
    bool bMeshRotationActive = false;
    bool bRemoveHMDRollInVR = false;
    TMap<UParticleModule*, uint32> ModuleOffsetMap;
    TMap<UParticleModule*, uint32> ModuleInstanceOffsetMap;
    TMap<UParticleModule*, uint32> ModuleRandomSeedInstanceOffsetMap;
    TArray<UMaterialInterface*> MeshMaterials;
    int32 DynamicParameterDataOffset = 0;
    int32 LightDataOffset = 0;
    float LightVolumetricScatteringIntensity = 0;
    int32 CameraPayloadOffset = 0;
    int32 ParticleSize = 0;
    int32 ReqInstanceBytes = 0;
    FVector2D PivotOffset = FVector2D(-0.5f, -0.5f);
    int32 TypeDataOffset = 0;
    int32 TypeDataInstanceOffset = -1;
    float MinFacingCameraBlendDistance = 0;
    float MaxFacingCameraBlendDistance = 0;
    TArray<UParticleModule*> ModulesNeedingInstanceData;
    TArray<UParticleModule*> ModulesNeedingRandomSeedInstanceData;
    USubUVAnimation* SubUVAnimation = nullptr;

    void PostLoad() override;
    virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent) { return nullptr; }
    virtual void UpdateModuleLists();
    UParticleLODLevel* GetCurrentLODLevel(FParticleEmitterInstance* Instance);
    UParticleLODLevel* GetLODLevel(int32 LODLevel) const { return LODLevel < LODLevels.Num() ? LODLevels[LODLevel].Get() : nullptr; }
    void Build();
    void CacheEmitterModuleInfo();
    float GetQualityLevelSpawnRateMult() const { return 1.0f; }
    bool HasAnyEnabledLODs() const;
};

UCLASS()
class UParticleSpriteEmitter : public UParticleEmitter {
    GENERATED_BODY()
public:
    void PostLoad() override;
    FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent) override;
};

}
