#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleKill.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleKillBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleKillBox : public UParticleModuleKillBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleKillBox(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector LowerLeftCorner;
    UPROPERTY()
    FRawDistributionVector UpperRightCorner;
    UPROPERTY()
    bool bAbsolute = false;
    UPROPERTY()
    bool bKillInside = false;
    UPROPERTY()
    bool bAxisAlignedAndFixedSize = true;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

UCLASS()
class UParticleModuleKillHeight : public UParticleModuleKillBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleKillHeight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat Height;
    UPROPERTY()
    bool bAbsolute = false;
    UPROPERTY()
    bool bFloor = false;
    UPROPERTY()
    bool bApplyPSysScale = false;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

}
