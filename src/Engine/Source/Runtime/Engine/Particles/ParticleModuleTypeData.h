#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleModuleTypeData.generated.h"

namespace ue5parse::runtime {

class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleSpawnPerUnit;
class UStaticMesh;
class UVectorField;

UENUM()
enum EMeshScreenAlignment : int32 { PSMA_MeshFaceCameraWithRoll, PSMA_MeshFaceCameraWithSpin, PSMA_MeshFaceCameraWithLockedAxis, PSMA_MAX };

UENUM()
enum EMeshCameraFacingUpAxis : int32 {
    CameraFacing_NoneUP,
    CameraFacing_ZUp,
    CameraFacing_NegativeZUp,
    CameraFacing_YUp,
    CameraFacing_NegativeYUp,
    CameraFacing_MAX,
};

UENUM()
enum EMeshCameraFacingOptions : int32 {
    XAxisFacing_NoUp,
    XAxisFacing_ZUp,
    XAxisFacing_NegativeZUp,
    XAxisFacing_YUp,
    XAxisFacing_NegativeYUp,
    LockedAxis_ZAxisFacing,
    LockedAxis_NegativeZAxisFacing,
    LockedAxis_YAxisFacing,
    LockedAxis_NegativeYAxisFacing,
    VelocityAligned_ZAxisFacing,
    VelocityAligned_NegativeZAxisFacing,
    VelocityAligned_YAxisFacing,
    VelocityAligned_NegativeYAxisFacing,
    EMeshCameraFacingOptions_MAX,
};

UENUM()
enum ETrailsRenderAxisOption : int32 { Trails_CameraUp, Trails_SourceUp, Trails_WorldUp, Trails_MAX };

UENUM()
enum EBeam2Method : int32 { PEB2M_Distance, PEB2M_Target, PEB2M_Branch, PEB2M_MAX };

UENUM()
enum EBeamTaperMethod : int32 { PEBTM_None, PEBTM_Full, PEBTM_Partial, PEBTM_MAX };

UCLASS(abstract)
class UParticleModuleTypeDataBase : public UParticleModule {
    GENERATED_BODY()
public:
    EModuleType GetModuleType() const override { return EPMT_TypeData; }
    virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) { return nullptr; }
    virtual void CacheModuleInfo(UParticleEmitter* Emitter) {}
    virtual bool SupportsSpecificScreenAlignmentFlags() const { return false; }
    virtual bool SupportsSubUV() const { return false; }
    virtual bool IsAMeshEmitter() const { return false; }
    virtual bool IsMotionBlurEnabled() const { return false; }
};

UCLASS()
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UStaticMesh> Mesh = nullptr;
    UPROPERTY()
    float LODSizeScale = 1.0f;
    UPROPERTY()
    bool bIsDecal = false;
    UPROPERTY()
    uint32 DecalSortOrder = 0;
    UPROPERTY()
    uint32 DecalPoolCount = 0;
    UPROPERTY()
    FName DecalColorName;
    UPROPERTY()
    FName DecalDynamicParameterName;
    UPROPERTY()
    bool bNeverOutline = false;
    UPROPERTY()
    bool bUseStaticMeshLODs = true;
    UPROPERTY()
    bool CastShadows = false;
    UPROPERTY()
    bool DoCollisions = false;
    UPROPERTY()
    TEnumAsByte<EMeshScreenAlignment> MeshAlignment = PSMA_MeshFaceCameraWithRoll;
    UPROPERTY()
    bool bOverrideMaterial = false;
    UPROPERTY()
    bool bOverrideDefaultMotionBlurSettings = false;
    UPROPERTY()
    bool bEnableMotionBlur = false;
    UPROPERTY()
    FRawDistributionVector RollPitchYawRange;
    UPROPERTY()
    TEnumAsByte<EParticleAxisLock> AxisLockOption = EPAL_NONE;
    UPROPERTY()
    bool bCameraFacing = false;
    UPROPERTY()
    TEnumAsByte<EMeshCameraFacingUpAxis> CameraFacingUpAxisOption = CameraFacing_NoneUP;
    UPROPERTY()
    TEnumAsByte<EMeshCameraFacingOptions> CameraFacingOption = XAxisFacing_NoUp;
    UPROPERTY()
    bool bScaleDuringCameraFacing = false;
    UPROPERTY()
    bool bApplyParticleRotationAsSpin = false;
    UPROPERTY()
    bool bFaceCameraDirectionRatherThanPosition = false;
    UPROPERTY()
    bool bCollisionsConsiderPartilceSize = true;

    FRandomStream RandomStream;

    void PostLoad() override;
    FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) override;
    bool SupportsSpecificScreenAlignmentFlags() const override { return true; }
    bool SupportsSubUV() const override { return true; }
    bool IsAMeshEmitter() const override { return true; }
    bool IsMotionBlurEnabled() const override { return bOverrideDefaultMotionBlurSettings && bEnableMotionBlur; }
};

UCLASS()
class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 MaxTessellationBetweenParticles = 25;
    UPROPERTY()
    int32 SheetsPerTrail = 1;
    UPROPERTY()
    int32 MaxTrailCount = 1;
    UPROPERTY()
    int32 MaxParticleInTrailCount = 500;
    UPROPERTY()
    bool bDeadTrailsOnDeactivate = true;
    UPROPERTY()
    bool bDeadTrailsOnSourceLoss = true;
    UPROPERTY()
    bool bClipSourceSegement = true;
    UPROPERTY()
    bool bEnablePreviousTangentRecalculation = true;
    UPROPERTY()
    bool bTangentRecalculationEveryFrame = false;
    UPROPERTY()
    bool bSpawnInitialParticle = false;
    UPROPERTY()
    TEnumAsByte<ETrailsRenderAxisOption> RenderAxis = Trails_CameraUp;
    UPROPERTY()
    float TangentSpawningScalar = 0.0f;
    UPROPERTY()
    bool bRenderGeometry = true;
    UPROPERTY()
    bool bRenderSpawnPoints = false;
    UPROPERTY()
    bool bRenderTangents = false;
    UPROPERTY()
    bool bRenderTessellation = false;
    UPROPERTY()
    float TilingDistance = 0.0f;
    UPROPERTY()
    float DistanceTessellationStepSize = 15.0f;
    UPROPERTY()
    bool bEnableTangentDiffInterpScale = false;
    UPROPERTY()
    float TangentTessellationScalar = 5.0f;
};

USTRUCT()
struct FBeamTargetData {
    GENERATED_BODY()
    UPROPERTY()
    FName TargetName;
    UPROPERTY()
    float TargetPercentage = 0.0f;
};

UCLASS()
class UParticleModuleTypeDataBeam2 : public UParticleModuleTypeDataBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EBeam2Method> BeamMethod = PEB2M_Target;
    UPROPERTY()
    int32 TextureTile = 1;
    UPROPERTY()
    float TextureTileDistance = 0.0f;
    UPROPERTY()
    int32 Sheets = 1;
    UPROPERTY()
    int32 MaxBeamCount = 0;
    UPROPERTY()
    float Speed = 10.0f;
    UPROPERTY()
    int32 InterpolationPoints = 0;
    UPROPERTY()
    bool bAlwaysOn = false;
    UPROPERTY()
    int32 UpVectorStepSize = 0;
    UPROPERTY()
    FName BranchParentName;
    UPROPERTY()
    FRawDistributionFloat Distance;
    UPROPERTY()
    TEnumAsByte<EBeamTaperMethod> TaperMethod = PEBTM_None;
    UPROPERTY()
    FRawDistributionFloat TaperFactor;
    UPROPERTY()
    FRawDistributionFloat TaperScale;
    UPROPERTY()
    bool RenderGeometry = true;
    UPROPERTY()
    bool RenderDirectLine = false;
    UPROPERTY()
    bool RenderLines = false;
    UPROPERTY()
    bool RenderTessellation = false;

    FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

USTRUCT()
struct FGPUSpriteLocalVectorFieldInfo {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UVectorField> Field = nullptr;
    UPROPERTY()
    FTransform Transform;
    UPROPERTY()
    FRotator MinInitialRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FRotator MaxInitialRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FRotator RotationRate = FRotator::ZeroRotator;
    UPROPERTY()
    float Intensity = 0.0f;
    UPROPERTY()
    float Tightness = 0.0f;
    UPROPERTY()
    bool bIgnoreComponentTransform = false;
    UPROPERTY()
    bool bTileX = false;
    UPROPERTY()
    bool bTileY = false;
    UPROPERTY()
    bool bTileZ = false;
    UPROPERTY()
    bool bUseFixDT = false;
};

USTRUCT()
struct FGPUSpriteEmitterInfo {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UParticleModuleRequired> RequiredModule = nullptr;
    UPROPERTY()
    TObjectPtr<UParticleModuleSpawn> SpawnModule = nullptr;
    UPROPERTY()
    TObjectPtr<UParticleModuleSpawnPerUnit> SpawnPerUnitModule = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UParticleModule>> SpawnModules;
    UPROPERTY()
    FGPUSpriteLocalVectorFieldInfo LocalVectorField;
    UPROPERTY()
    FFloatDistribution VectorFieldScale;
    UPROPERTY()
    FFloatDistribution DragCoefficient;
    UPROPERTY()
    FFloatDistribution PointAttractorStrength;
    UPROPERTY()
    FFloatDistribution Resilience;
    UPROPERTY()
    FVector ConstantAcceleration = FVector::ZeroVector;
    UPROPERTY()
    FVector PointAttractorPosition = FVector::ZeroVector;
    UPROPERTY()
    float PointAttractorRadiusSq = 0.0f;
    UPROPERTY()
    FVector OrbitOffsetBase = FVector::ZeroVector;
    UPROPERTY()
    FVector OrbitOffsetRange = FVector::ZeroVector;
    UPROPERTY()
    FVector2D InvMaxSize = FVector2D::ZeroVector;
    UPROPERTY()
    float InvRotationRateScale = 0.0f;
    UPROPERTY()
    float MaxLifetime = 0.0f;
    UPROPERTY()
    int32 MaxParticleCount = 0;
    UPROPERTY()
    TEnumAsByte<EParticleScreenAlignment> ScreenAlignment = PSA_Square;
    UPROPERTY()
    TEnumAsByte<EParticleAxisLock> LockAxisFlag = EPAL_NONE;
    UPROPERTY()
    bool bEnableCollision = false;
    UPROPERTY()
    TEnumAsByte<EParticleCollisionMode::Type> CollisionMode = EParticleCollisionMode::SceneDepth;
    UPROPERTY()
    bool bUseVelocityForMotionBlur = false;
    UPROPERTY()
    bool bRemoveHMDRoll = false;
    UPROPERTY()
    float MinFacingCameraBlendDistance = 0.0f;
    UPROPERTY()
    float MaxFacingCameraBlendDistance = 0.0f;
    UPROPERTY()
    FRawDistributionVector DynamicColor;
    UPROPERTY()
    FRawDistributionFloat DynamicAlpha;
    UPROPERTY()
    FRawDistributionVector DynamicColorScale;
    UPROPERTY()
    FRawDistributionFloat DynamicAlphaScale;
};

USTRUCT()
struct FGPUSpriteResourceData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FColor> QuantizedColorSamples;
    UPROPERTY()
    TArray<FColor> QuantizedMiscSamples;
    UPROPERTY()
    TArray<FColor> QuantizedSimulationAttrSamples;
    UPROPERTY()
    FVector4 ColorScale;
    UPROPERTY()
    FVector4 ColorBias;
    UPROPERTY()
    FVector4 MiscScale;
    UPROPERTY()
    FVector4 MiscBias;
    UPROPERTY()
    FVector4 SimulationAttrCurveScale;
    UPROPERTY()
    FVector4 SimulationAttrCurveBias;
    UPROPERTY()
    FVector4 SubImageSize;
    UPROPERTY()
    FVector4 SizeBySpeed;
    UPROPERTY()
    FVector ConstantAcceleration;
    UPROPERTY()
    FVector OrbitOffsetBase;
    UPROPERTY()
    FVector OrbitOffsetRange;
    UPROPERTY()
    FVector OrbitFrequencyBase;
    UPROPERTY()
    FVector OrbitFrequencyRange;
    UPROPERTY()
    FVector OrbitPhaseBase;
    UPROPERTY()
    FVector OrbitPhaseRange;
    UPROPERTY()
    float GlobalVectorFieldScale = 0.0f;
    UPROPERTY()
    float GlobalVectorFieldTightness = 0.0f;
    UPROPERTY()
    float PerParticleVectorFieldScale = 0.0f;
    UPROPERTY()
    float PerParticleVectorFieldBias = 0.0f;
    UPROPERTY()
    float DragCoefficientScale = 0.0f;
    UPROPERTY()
    float DragCoefficientBias = 0.0f;
    UPROPERTY()
    float ResilienceScale = 0.0f;
    UPROPERTY()
    float ResilienceBias = 0.0f;
    UPROPERTY()
    float CollisionRadiusScale = 0.0f;
    UPROPERTY()
    float CollisionRadiusBias = 0.0f;
    UPROPERTY()
    float CollisionTimeBias = 0.0f;
    UPROPERTY()
    float CollisionRandomSpread = 0.0f;
    UPROPERTY()
    float CollisionRandomDistribution = 0.0f;
    UPROPERTY()
    float OneMinusFriction = 0.0f;
    UPROPERTY()
    float RotationRateScale = 0.0f;
    UPROPERTY()
    float CameraMotionBlurAmount = 0.0f;
    UPROPERTY()
    TEnumAsByte<EParticleScreenAlignment> ScreenAlignment = PSA_Square;
    UPROPERTY()
    TEnumAsByte<EParticleAxisLock> LockAxisFlag = EPAL_NONE;
    UPROPERTY()
    FVector2D PivotOffset;
    UPROPERTY()
    bool bUseVelocityForMotionBlur = false;
    UPROPERTY()
    bool bRemoveHMDRoll = false;
    UPROPERTY()
    float MinFacingCameraBlendDistance = 0.0f;
    UPROPERTY()
    float MaxFacingCameraBlendDistance = 0.0f;
};

UCLASS()
class UParticleModuleTypeDataGpu : public UParticleModuleTypeDataBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGPUSpriteEmitterInfo EmitterInfo;
    UPROPERTY()
    FGPUSpriteResourceData ResourceData;
    UPROPERTY()
    float CameraMotionBlurAmount = 0.0f;
    UPROPERTY()
    bool bClearExistingParticlesOnInit = false;
};

}
