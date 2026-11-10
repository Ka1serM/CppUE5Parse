#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Matrix.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleHelper.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UParticleLODLevel;
class UParticleModuleTypeDataMesh;
class UParticleSystemComponent;
class UWorld;

struct FLODBurstFired {
    TArray<bool> Fired;
};

struct FParticleEmitterInstance {
    static constexpr float PeakActiveParticleUpdateDelta = 0.05f;
    static constexpr int32 MaxCPUParticlesPerEmitter = 1000;

    UParticleEmitter* SpriteTemplate = nullptr;
    UParticleSystemComponent* Component = nullptr;
    int32 CurrentLODLevelIndex = 0;
    UParticleLODLevel* CurrentLODLevel = nullptr;
    int32 TypeDataOffset = 0;
    int32 TypeDataInstanceOffset = -1;
    int32 SubUVDataOffset = 0;
    int32 DynamicParameterDataOffset = 0;
    int32 LightDataOffset = 0;
    float LightVolumetricScatteringIntensity = 0;
    int32 OrbitModuleOffset = 0;
    int32 CameraPayloadOffset = 0;
    FVector Location = FVector::ZeroVector;
    FMatrix EmitterToSimulation;
    FMatrix SimulationToWorld;
    bool bEnabled = true;
    bool bKillOnDeactivate = false;
    bool bKillOnCompleted = false;
    bool bHaltSpawning = false;
    bool bHaltSpawningExternal = false;
    bool bRequiresLoopNotification = false;
    bool bAxisLockEnabled = false;
    bool bFakeBurstsWhenSpawningSupressed = false;
    TEnumAsByte<EParticleAxisLock> LockAxisFlags = EPAL_NONE;
    int32 SortMode = 0;
    uint8* ParticleData = nullptr;
    uint16* ParticleIndices = nullptr;
    uint8* InstanceData = nullptr;
    int32 InstancePayloadSize = 0;
    int32 PayloadOffset = 0;
    int32 ParticleSize = 0;
    int32 ParticleStride = 0;
    int32 ActiveParticles = 0;
    uint32 ParticleCounter = 0;
    int32 MaxActiveParticles = 0;
    float SpawnFraction = 0;
    float SecondsSinceCreation = 0;
    float EmitterTime = 0;
    float LastDeltaTime = 0;
    FVector OldLocation = FVector::ZeroVector;
    FBox ParticleBoundingBox;
    TArray<FLODBurstFired> BurstFired;
    int32 LoopCount = 0;
    float EmitterDuration = 0;
    TArray<float> EmitterDurations;
    float CurrentDelay = 0;
    int32 PreciseLifetimeOffset = 0;
    bool bEmitterIsDone = false;
    bool bHasCustomColor = false;
    UMaterialInterface* CurrentMaterial = nullptr;
    FVector PositionOffsetThisTick = FVector::ZeroVector;
    FVector2D PivotOffset = FVector2D(-0.5, -0.5);

    FParticleEmitterInstance() = default;
    FParticleEmitterInstance(const FParticleEmitterInstance&) = delete;
    FParticleEmitterInstance& operator=(const FParticleEmitterInstance&) = delete;
    virtual ~FParticleEmitterInstance();

    virtual void InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent);
    virtual void Init();
    UWorld* GetWorld() const;
    virtual bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = true);
    virtual void Tick(float DeltaTime, bool bSuppressSpawning);
    void CheckEmitterFinished();
    void FakeBursts();
    virtual float Tick_EmitterTimeSetup(float DeltaTime, UParticleLODLevel* InCurrentLODLevel);
    virtual float Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* InCurrentLODLevel, bool bSuppressSpawning, bool bFirstTime);
    virtual void Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel);
    virtual void Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel);
    virtual void Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel);
    virtual void SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess);
    virtual void Rewind();
    virtual FBox GetBoundingBox() { return ParticleBoundingBox; }
    virtual void UpdateBoundingBox(float DeltaTime);
    virtual uint32 RequiredBytes();
    uint32 GetModuleDataOffset(UParticleModule* Module);
    uint8* GetModuleInstanceData(UParticleModule* Module);
    FParticleRandomSeedInstancePayload* GetModuleRandomSeedInstanceData(UParticleModule* Module);
    virtual uint32 CalculateParticleStride(uint32 InParticleSize) { return InParticleSize; }
    virtual void ResetBurstList();
    virtual float GetCurrentBurstRateOffset(float& DeltaTime, int32& Burst);
    virtual void ResetParticleParameters(float DeltaTime);
    void CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload, FVector& AccumOffset, FVector& AccumRotation, FVector& AccumRotationRate,
                              float DeltaTime, FVector& Result, FMatrix& RotationMat);
    virtual void UpdateOrbitData(float DeltaTime);
    virtual float Spawn(float DeltaTime);
    void SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity,
                        FParticleEventInstancePayload* EventPayload);
    virtual void ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity);
    virtual void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity);
    virtual void PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime);
    virtual bool HasCompleted();
    void SetPreciseLifetimeOffset(int32 InOffset) { PreciseLifetimeOffset = InOffset; }
    virtual void KillParticles();
    virtual void KillParticle(int32 Index);
    virtual void KillParticlesForced(bool bFireEvents = false);
    virtual void SetHaltSpawning(bool bInHaltSpawning) { bHaltSpawning = bInHaltSpawning; }
    virtual void SetHaltSpawningExternal(bool bInHaltSpawning) { bHaltSpawningExternal = bInHaltSpawning; }
    int32 GetOrbitPayloadOffset();
    FVector GetParticleLocationWithOrbitOffset(FBaseParticle* Particle);
    virtual FBaseParticle* GetParticle(int32 Index);
    void SetupEmitterDuration();
    bool HasActiveParticles() const { return ActiveParticles > 0; }
    virtual bool IsDynamicDataRequired(UParticleLODLevel* InCurrentLODLevel);
    virtual void ProcessParticleEvents(float DeltaTime, bool bSuppressSpawning);
    virtual void OnDeactivateSystem() {}
    virtual int32 GetMeshRotationOffset() const { return 0; }
    virtual bool IsMeshRotationActive() const { return false; }
    virtual void SetMeshMaterials(const TArray<UMaterialInterface*>& InMaterials) {}
    virtual void OnEmitterInstanceKilled(FParticleEmitterInstance* Instance) {}
    virtual bool IsTrailEmitter() const { return false; }
    virtual void Tick_MaterialOverrides(int32 EmitterIndex);
    void InitCustomColor();
    void ApplyCustomColor(UMaterialInstanceDynamic* MID);
    UParticleLODLevel* GetCurrentLODLevelChecked();

protected:
    void UpdateTransforms();
    void FixupParticleIndices();
};

struct FParticleSpriteEmitterInstance : FParticleEmitterInstance {};

struct FParticleMeshEmitterInstance : FParticleEmitterInstance {
    UParticleModuleTypeDataMesh* MeshTypeData = nullptr;
    bool MeshRotationActive = false;
    int32 MeshRotationOffset = 0;
    int32 MeshMotionBlurOffset = 0;
    TArray<UMaterialInterface*> CurrentMaterials;

    void InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) override;
    bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = true) override;
    void Tick(float DeltaTime, bool bSuppressSpawning) override;
    void UpdateBoundingBox(float DeltaTime) override;
    uint32 RequiredBytes() override;
    void PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime) override;
    bool IsDynamicDataRequired(UParticleLODLevel* InCurrentLODLevel) override;
    void Tick_MaterialOverrides(int32 EmitterIndex) override;
    int32 GetMeshRotationOffset() const override { return MeshRotationOffset; }
    bool IsMeshRotationActive() const override { return MeshRotationActive; }
    void SetMeshMaterials(const TArray<UMaterialInterface*>& InMaterials) override { CurrentMaterials = InMaterials; }
    void GetMeshMaterials(TArray<UMaterialInterface*>& OutMaterials, const UParticleLODLevel* LODLevel) const;
};

}
