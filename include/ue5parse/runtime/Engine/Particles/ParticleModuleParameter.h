#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleParameter.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EEmitterDynamicParameterValue : int {
    EDPV_UserSet,
    EDPV_AutoSet,
    EDPV_VelocityX,
    EDPV_VelocityY,
    EDPV_VelocityZ,
    EDPV_VelocityMag,
    EDPV_MAX,
};

USTRUCT()
struct FEmitterDynamicParameter {
    GENERATED_BODY()

    UPROPERTY()
    FName ParamName;
    UPROPERTY()
    bool bUseEmitterTime = false;
    UPROPERTY()
    bool bSpawnTimeOnly = false;
    UPROPERTY()
    TEnumAsByte<EEmitterDynamicParameterValue> ValueMethod = EDPV_UserSet;
    UPROPERTY()
    bool bScaleVelocityByParamValue = false;
    UPROPERTY()
    FRawDistributionFloat ParamValue;
};

// UParticleModuleParameterBase (UE's Particles/Parameter/ParticleModuleParameterBase.h).
UCLASS(abstract)
class UParticleModuleParameterBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleParameterDynamic: the four dynamic material parameters of each particle
// (UE's Particles/Parameter/ParticleModuleParameterDynamic.h).
UCLASS()
class UParticleModuleParameterDynamic : public UParticleModuleParameterBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleParameterDynamic(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FEmitterDynamicParameter> DynamicParams;
    UPROPERTY()
    int32 UpdateFlags = 0;
    UPROPERTY()
    bool bUsesVelocity = false;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);

    float GetParameterValue(FEmitterDynamicParameter& InDynParams, FBaseParticle& Particle, FParticleEmitterInstance* Owner, FRandomStream* InRandomStream);
    float GetParameterValue_UserSet(FEmitterDynamicParameter& InDynParams, FBaseParticle& Particle, FParticleEmitterInstance* Owner, FRandomStream* InRandomStream);
};

// UParticleModuleParameterDynamic_Seeded (UE's Particles/Parameter/ParticleModuleParameterDynamic_Seeded.h).
UCLASS()
class UParticleModuleParameterDynamic_Seeded : public UParticleModuleParameterDynamic {
    GENERATED_BODY()
public:
    explicit UParticleModuleParameterDynamic_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

}
