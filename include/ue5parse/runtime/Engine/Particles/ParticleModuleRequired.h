#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"
#include "ue5parse/runtime/Engine/Particles/SubUVAnimation.h"

#include "ParticleModuleRequired.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UTexture2D;

UENUM()
enum EParticleSortMode : int32 {
    PSORTMODE_None,
    PSORTMODE_ViewProjDepth,
    PSORTMODE_DistanceToView,
    PSORTMODE_Age_OldestFirst,
    PSORTMODE_Age_NewestFirst,
    PSORTMODE_MAX,
};

UENUM()
enum EEmitterNormalsMode : int32 { ENM_CameraFacing, ENM_Spherical, ENM_Cylindrical, ENM_MAX };

UENUM()
enum class EParticleUVFlipMode : uint8 {
    None,
    FlipUV,
    FlipUOnly,
    FlipVOnly,
    RandomFlipUV,
    RandomFlipUOnly,
    RandomFlipVOnly,
    RandomFlipUVIndependent,
};

inline void AdjustParticleBaseSizeForUVFlipping(FVector& OutSize, EParticleUVFlipMode FlipMode, FRandomStream& InRandomStream) {
    static const float HalfRandMax = 0.5f;

    switch (FlipMode) {
    case EParticleUVFlipMode::None:
        return;
    case EParticleUVFlipMode::FlipUV:
        OutSize = -OutSize;
        return;
    case EParticleUVFlipMode::FlipUOnly:
        OutSize.X = -OutSize.X;
        return;
    case EParticleUVFlipMode::FlipVOnly:
        OutSize.Y = -OutSize.Y;
        return;
    case EParticleUVFlipMode::RandomFlipUV:
        OutSize = InRandomStream.FRand() > HalfRandMax ? -OutSize : OutSize;
        return;
    case EParticleUVFlipMode::RandomFlipUOnly:
        OutSize.X = InRandomStream.FRand() > HalfRandMax ? -OutSize.X : OutSize.X;
        return;
    case EParticleUVFlipMode::RandomFlipVOnly:
        OutSize.Y = InRandomStream.FRand() > HalfRandMax ? -OutSize.Y : OutSize.Y;
        return;
    case EParticleUVFlipMode::RandomFlipUVIndependent:
        OutSize.X = InRandomStream.FRand() > HalfRandMax ? -OutSize.X : OutSize.X;
        OutSize.Y = InRandomStream.FRand() > HalfRandMax ? -OutSize.Y : OutSize.Y;
        return;
    }
}

// FParticleRequiredModule's cooked cutout geometry: the bounding polygon of each sub image (UE's
// FSubUVDerivedData).
struct FSubUVDerivedData {
    TArray<FVector2f> BoundingGeometry;
};

// UParticleModuleRequired: the settings every emitter LOD level has: material, duration and loops, sub images,
// alignment and space (UE's Particles/ParticleModuleRequired.h).
UCLASS()
class UParticleModuleRequired : public UParticleModule {
    GENERATED_BODY()
public:
    explicit UParticleModuleRequired(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    float MinFacingCameraBlendDistance = 0;
    UPROPERTY()
    float MaxFacingCameraBlendDistance = 0;
    UPROPERTY()
    FVector EmitterOrigin = FVector::ZeroVector;
    UPROPERTY()
    FRotator EmitterRotation = FRotator::ZeroRotator;
    UPROPERTY()
    TEnumAsByte<EParticleScreenAlignment> ScreenAlignment = PSA_Square;
    UPROPERTY()
    bool bUseLocalSpace = false;
    UPROPERTY()
    bool bKillOnDeactivate = false;
    UPROPERTY()
    bool bKillOnCompleted = false;
    UPROPERTY()
    TEnumAsByte<EParticleSortMode> SortMode = PSORTMODE_None;
    UPROPERTY()
    bool bUseLegacyEmitterTime = true;
    UPROPERTY()
    bool bRemoveHMDRoll = false;
    UPROPERTY()
    bool bSupportLargeWorldCoordinates = true;
    UPROPERTY()
    bool bEmitterDurationUseRange = false;
    UPROPERTY()
    bool bOverrideUseVelocityForMotionBlur = false;
    UPROPERTY()
    bool bUseVelocityForMotionBlur = false;
    UPROPERTY()
    float EmitterDuration = 1.0f;
    UPROPERTY()
    FRawDistributionFloat SpawnRate;
    UPROPERTY()
    TArray<FParticleBurst> BurstList;
    UPROPERTY()
    float EmitterDelay = 0;
    UPROPERTY()
    float EmitterDelayLow = 0;
    UPROPERTY()
    bool bDelayFirstLoopOnly = false;
    UPROPERTY()
    TEnumAsByte<EParticleSubUVInterpMethod> InterpolationMethod = PSUVIM_None;
    UPROPERTY()
    bool bScaleUV = false;
    UPROPERTY()
    bool bEmitterDelayUseRange = false;
    UPROPERTY()
    TEnumAsByte<EParticleBurstMethod> ParticleBurstMethod = EPBM_Instant;
    UPROPERTY()
    bool bOverrideSystemMacroUV = false;
    UPROPERTY()
    bool bUseMaxDrawCount = true;
    UPROPERTY()
    TEnumAsByte<EOpacitySourceMode> OpacitySourceMode = OSM_Alpha;
    UPROPERTY()
    TEnumAsByte<EEmitterNormalsMode> EmitterNormalsMode = ENM_CameraFacing;
    UPROPERTY()
    bool bOrbitModuleAffectsVelocityAlignment = false;
    UPROPERTY()
    int32 SubImages_Horizontal = 1;
    UPROPERTY()
    int32 SubImages_Vertical = 1;
    UPROPERTY()
    float RandomImageTime = 0;
    UPROPERTY()
    int32 RandomImageChanges = 0;
    UPROPERTY()
    FVector MacroUVPosition = FVector::ZeroVector;
    UPROPERTY()
    float MacroUVRadius = 0;
    UPROPERTY()
    EParticleUVFlipMode UVFlippingMode = EParticleUVFlipMode::None;
    UPROPERTY()
    TEnumAsByte<ESubUVBoundingVertexCount> BoundingMode = BVC_EightVertices;
    UPROPERTY()
    bool bDurationRecalcEachLoop = false;
    UPROPERTY()
    FVector NormalsSphereCenter = FVector(0, 0, 100);
    UPROPERTY()
    float AlphaThreshold = 0.1f;
    UPROPERTY()
    int32 EmitterLoops = 0;
    UPROPERTY()
    TObjectPtr<UTexture2D> CutoutTexture = nullptr;
    UPROPERTY()
    int32 MaxDrawCount = 500;
    UPROPERTY()
    float EmitterDurationLow = 0;
    UPROPERTY()
    FVector NormalsCylinderDirection = FVector(0, 0, 1);
    UPROPERTY()
    TArray<FName> NamedMaterialOverrides;

    FSubUVDerivedData DerivedData;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
    EModuleType GetModuleType() const override { return EPMT_Required; }
    bool ShouldUseVelocityForMotionBlur() const { return bOverrideUseVelocityForMotionBlur ? bUseVelocityForMotionBlur : true; }
    int32 GetNumFrames() const { return SubImages_Vertical * SubImages_Horizontal; }
    int32 GetNumBoundingVertices() const { return BoundingMode == BVC_FourVertices ? 4 : 8; }
    bool IsBoundingGeometryValid() const { return DerivedData.BoundingGeometry.Num() != 0; }
};

}
