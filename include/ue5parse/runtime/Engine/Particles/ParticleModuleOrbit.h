#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleOrbit.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EOrbitChainMode : int {
    EOChainMode_Add,
    EOChainMode_Scale,
    EOChainMode_Link,
    EOChainMode_MAX,
};

USTRUCT()
struct FOrbitOptions {
    GENERATED_BODY()

    UPROPERTY()
    bool bProcessDuringSpawn = true;
    UPROPERTY()
    bool bProcessDuringUpdate = false;
    UPROPERTY()
    bool bUseEmitterTime = false;
};

// UParticleModuleOrbitBase (UE's Particles/Orbit/ParticleModuleOrbitBase.h).
UCLASS(abstract)
class UParticleModuleOrbitBase : public UParticleModule {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bUseEmitterTime = false;
};

// UParticleModuleOrbit: moves each particle on an orbit around its location; the emitter instance chains the orbit
// modules (UE's Particles/Orbit/ParticleModuleOrbit.h).
UCLASS()
class UParticleModuleOrbit : public UParticleModuleOrbitBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleOrbit(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<EOrbitChainMode> ChainMode = EOChainMode_Add;
    UPROPERTY()
    FRawDistributionVector OffsetAmount;
    UPROPERTY()
    FOrbitOptions OffsetOptions;
    UPROPERTY()
    FRawDistributionVector RotationAmount;
    UPROPERTY()
    FOrbitOptions RotationOptions;
    UPROPERTY()
    FRawDistributionVector RotationRateAmount;
    UPROPERTY()
    FOrbitOptions RotationRateOptions;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

}
