#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

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

UCLASS(abstract)
class UParticleModuleParameterBase : public UParticleModule {
    GENERATED_BODY()
};

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
    float GetParameterValue_UserSet(FEmitterDynamicParameter& InDynParams, FBaseParticle& Particle, FParticleEmitterInstance* Owner,
                                    FRandomStream* InRandomStream);
};

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
