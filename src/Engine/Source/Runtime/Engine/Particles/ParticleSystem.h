#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleSystem.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UParticleSystemComponent;

UENUM()
enum class EParticleSystemInsignificanceReaction : uint8 { Auto, Complete, DisableTick, DisableTickAndKill, Num };

UENUM()
enum EParticleSystemUpdateMode : int32 { EPSUM_RealTime, EPSUM_FixedTime };

UENUM()
enum ParticleSystemLODMethod : int32 {
    PARTICLESYSTEMLODMETHOD_Automatic,
    PARTICLESYSTEMLODMETHOD_DirectSet,
    PARTICLESYSTEMLODMETHOD_ActivateAutomatic,
};

UENUM()
enum EParticleSystemOcclusionBoundsMethod : int32 { EPSOBM_None, EPSOBM_ParticleBounds, EPSOBM_CustomBounds };

USTRUCT()
struct FParticleSystemLOD {
    GENERATED_BODY()
};

USTRUCT()
struct FLODSoloTrack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> SoloEnableSetting;
};

USTRUCT()
struct FNamedEmitterMaterial {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

UCLASS(abstract)
class UFXSystemAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 MaxPoolSize = 32;
    UPROPERTY()
    uint32 PoolPrimeSize = 0;
};

UCLASS()
class UParticleSystem : public UFXSystemAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bGreyscalable = false;
    UPROPERTY()
    float UpdateTime_FPS = 60.0f;
    UPROPERTY()
    float UpdateTime_Delta = 1.0f / 60.0f;
    UPROPERTY()
    float WarmupTime = 0;
    UPROPERTY()
    float WarmupTickRate = 0;
    UPROPERTY()
    TArray<TObjectPtr<UParticleEmitter>> Emitters;
    UPROPERTY()
    TObjectPtr<UParticleSystemComponent> PreviewComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> CurveEdSetup = nullptr;
    UPROPERTY()
    float LODDistanceCheckTime = 0.25f;
    UPROPERTY()
    float MacroUVRadius = 200.0f;
    UPROPERTY()
    TArray<float> LODDistances;
    UPROPERTY()
    TArray<FParticleSystemLOD> LODSettings;
    UPROPERTY()
    FBox FixedRelativeBoundingBox = FBox(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f));
    UPROPERTY()
    float SecondsBeforeInactive = 0;
    UPROPERTY()
    float Delay = 0;
    UPROPERTY()
    float DelayLow = 0;
    UPROPERTY()
    bool bOrientZAxisTowardCamera = false;
    UPROPERTY()
    bool bUseFixedRelativeBoundingBox = false;
    UPROPERTY()
    bool bShouldResetPeakCounts = false;
    UPROPERTY()
    bool bHasPhysics = false;
    UPROPERTY()
    bool bUseRealtimeThumbnail = false;
    UPROPERTY()
    bool ThumbnailImageOutOfDate = true;
    UPROPERTY()
    bool bUseDelayRange = false;
    UPROPERTY()
    bool bAllowManagedTicking = true;
    UPROPERTY()
    bool bAutoDeactivate = true;
    UPROPERTY()
    bool bRegenerateLODDuplicate = false;
    UPROPERTY()
    TEnumAsByte<EParticleSystemUpdateMode> SystemUpdateMode = EPSUM_RealTime;
    UPROPERTY()
    TEnumAsByte<ParticleSystemLODMethod> LODMethod = PARTICLESYSTEMLODMETHOD_Automatic;
    UPROPERTY()
    EParticleSystemInsignificanceReaction InsignificantReaction = EParticleSystemInsignificanceReaction::Auto;
    UPROPERTY()
    TEnumAsByte<EParticleSystemOcclusionBoundsMethod> OcclusionBoundsMethod = EPSOBM_ParticleBounds;
    UPROPERTY()
    EParticleSignificanceLevel MaxSignificanceLevel = EParticleSignificanceLevel::Critical;
    UPROPERTY()
    uint32 MinTimeBetweenTicks = 0;
    UPROPERTY()
    float InsignificanceDelay = 0;
    UPROPERTY()
    FVector MacroUVPosition = FVector::ZeroVector;
    UPROPERTY()
    FBox CustomOcclusionBounds;
    UPROPERTY()
    TArray<FLODSoloTrack> SoloTracking;
    UPROPERTY()
    TArray<FNamedEmitterMaterial> NamedMaterialSlots;

    void PostLoad() override;
    int32 GetLODLevelCount() const { return LODDistances.Num(); }
    bool IsLooping() const { return bAnyEmitterLoopsForever; }
    //! An immortal emitter spawns nothing and the system does not deactivate: it would never complete.
    EParticleSignificanceLevel GetLowestSignificance() const { return LowestSignificance; }

private:
    bool bAnyEmitterLoopsForever = false;
    bool bIsImmortal = false;
    bool bWillBecomeZombie = false;
    bool bShouldManageSignificance = false;
    EParticleSignificanceLevel HighestSignificance = EParticleSignificanceLevel::Critical;
    EParticleSignificanceLevel LowestSignificance = EParticleSignificanceLevel::Low;
};

}
