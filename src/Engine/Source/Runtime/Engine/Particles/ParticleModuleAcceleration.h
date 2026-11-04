#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleAcceleration.generated.h"

namespace ue5parse::runtime {

class UParticleEmitter;
class UParticleLODLevel;
class UParticleModuleTypeDataBase;

UCLASS(abstract)
class UParticleModuleAccelerationBase : public UParticleModule {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bAlwaysInWorldSpace = false;
};

UCLASS()
class UParticleModuleAcceleration : public UParticleModuleAccelerationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAcceleration(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector Acceleration;
    UPROPERTY()
    bool bApplyOwnerScale = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

UCLASS()
class UParticleModuleAccelerationConstant : public UParticleModuleAccelerationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAccelerationConstant(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector Acceleration;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

UCLASS()
class UParticleModuleAccelerationDrag : public UParticleModuleAccelerationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAccelerationDrag(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDistributionFloat> DragCoefficient_DEPRECATED = nullptr;
    UPROPERTY()
    FRawDistributionFloat DragCoefficientRaw;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    void PostLoad() override;
};

UCLASS()
class UParticleModuleAccelerationDragScaleOverLife : public UParticleModuleAccelerationBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UDistributionFloat> DragScale_DEPRECATED = nullptr;
    UPROPERTY()
    FRawDistributionFloat DragScaleRaw;

    void PostLoad() override;
};

UCLASS()
class UParticleModuleAccelerationOverLifetime : public UParticleModuleAccelerationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAccelerationOverLifetime(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector AccelOverLife;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
