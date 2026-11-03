#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleCamera.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EParticleCameraOffsetUpdateMethod : int {
    EPCOUM_DirectSet,
    EPCOUM_Additive,
    EPCOUM_Scalar,
    EPCOUM_MAX,
};

// UParticleModuleCameraBase (UE's Particles/Camera/ParticleModuleCameraBase.h).
UCLASS(abstract)
class UParticleModuleCameraBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleCameraOffset: moves each particle along the camera direction (UE's Particles/Camera/ParticleModuleCameraOffset.h).
UCLASS()
class UParticleModuleCameraOffset : public UParticleModuleCameraBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleCameraOffset(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat CameraOffset;
    UPROPERTY()
    bool bSpawnTimeOnly = false;
    UPROPERTY()
    TEnumAsByte<EParticleCameraOffsetUpdateMethod> UpdateMethod = EPCOUM_DirectSet;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

}
