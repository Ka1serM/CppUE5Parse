#pragma once

#include "ue5parse/runtime/Core/Math/Box.h"
#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"

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

// FNamedEmitterMaterial: a material slot that emitters name and the component can override (UE's
// Particles/ParticleSystem.h).
USTRUCT()
struct FNamedEmitterMaterial {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

// UFXSystemAsset: the base of the particle system assets of Cascade and Niagara (UE's Particles/ParticleSystem.h).
UCLASS(abstract)
class UFXSystemAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 MaxPoolSize = 32;
    UPROPERTY()
    uint32 PoolPrimeSize = 0;
};

// UParticleSystem: a Cascade particle system: its emitters and their LOD settings (UE's Particles/ParticleSystem.h).
UCLASS()
class UParticleSystem : public UFXSystemAsset {
    GENERATED_BODY()
public:
    // Riot's: the system takes the greyscale variant colors.
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
    float GetLODDistance(int32 LODLevelIndex) const { return LODLevelIndex < LODDistances.Num() ? LODDistances[LODLevelIndex] : -1.0f; }
    ParticleSystemLODMethod GetCurrentLODMethod() const { return ParticleSystemLODMethod(LODMethod.GetValue()); }
    void BuildEmitters();
    bool UsesCPUCollision() const;
    bool ContainsEmitterType(UClass* TypeData) const;
    bool CanBePooled() const { return MaxPoolSize != 0; }
    // An emitter loops forever.
    bool IsLooping() const { return bAnyEmitterLoopsForever; }
    // An emitter loops forever with no duration: the system never completes by itself.
    bool IsImmortal() const { return bIsImmortal; }
    // An immortal emitter spawns nothing and the system does not deactivate: it would never complete.
    bool WillBecomeZombie() const { return bWillBecomeZombie; }
    EParticleSignificanceLevel GetHighestSignificance() const { return HighestSignificance; }
    EParticleSignificanceLevel GetLowestSignificance() const { return LowestSignificance; }
    bool ShouldManageSignificance() const { return bShouldManageSignificance; }

private:
    bool bAnyEmitterLoopsForever = false;
    bool bIsImmortal = false;
    bool bWillBecomeZombie = false;
    bool bShouldManageSignificance = false;
    EParticleSignificanceLevel HighestSignificance = EParticleSignificanceLevel::Critical;
    EParticleSignificanceLevel LowestSignificance = EParticleSignificanceLevel::Low;
};

}
