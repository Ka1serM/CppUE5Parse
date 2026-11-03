#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleEmitter.h"

#include "ParticleModuleTypeData.generated.h"

namespace ue5parse::runtime {

class UStaticMesh;

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

// UParticleModuleTypeDataBase: makes an emitter of another kind than camera facing sprites; it creates the emitter
// instance (UE's Particles/TypeData/ParticleModuleTypeDataBase.h).
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

// UParticleModuleTypeDataMesh: an emitter of static mesh particles (UE's
// Particles/TypeData/ParticleModuleTypeDataMesh.h). Riot adds mesh decals (bIsDecal), which this port keeps as data.
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

    // The stream of the initial orientations of the particles (RollPitchYawRange).
    FRandomStream RandomStream;

    void PostLoad() override;
    FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) override;
    bool SupportsSpecificScreenAlignmentFlags() const override { return true; }
    bool SupportsSubUV() const override { return true; }
    bool IsAMeshEmitter() const override { return true; }
    bool IsMotionBlurEnabled() const override { return bOverrideDefaultMotionBlurSettings && bEnableMotionBlur; }
};

}
