#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleAttractor.generated.h"

namespace ue5parse::runtime {

class UParticleModuleTypeDataBase;

// UParticleModuleAttractorBase (UE's Particles/Attractor/ParticleModuleAttractorBase.h).
UCLASS(abstract)
class UParticleModuleAttractorBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleAttractorPoint (UE's Particles/Attractor/ParticleModuleAttractorPoint.h).
UCLASS()
class UParticleModuleAttractorPoint : public UParticleModuleAttractorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAttractorPoint(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector Position;
    UPROPERTY()
    FRawDistributionFloat Range;
    UPROPERTY()
    FRawDistributionFloat Strength;
    UPROPERTY()
    bool StrengthByDistance = true;
    UPROPERTY()
    bool bAffectBaseVelocity = false;
    UPROPERTY()
    bool bOverrideVelocity = false;
    UPROPERTY()
    bool bUseWorldSpacePosition = false;
    UPROPERTY()
    bool Positive_X = true;
    UPROPERTY()
    bool Positive_Y = true;
    UPROPERTY()
    bool Positive_Z = true;
    UPROPERTY()
    bool Negative_X = true;
    UPROPERTY()
    bool Negative_Y = true;
    UPROPERTY()
    bool Negative_Z = true;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

UENUM()
enum EAttractorParticleSelectionMethod : int32 { EAPSM_Random, EAPSM_Sequential, EAPSM_MAX };
// UParticleModuleAttractorParticle (UE's Particles/Attractor/ParticleModuleAttractorParticle.h).
UCLASS()
class UParticleModuleAttractorParticle : public UParticleModuleAttractorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAttractorParticle(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName EmitterName = NAME_None;
    UPROPERTY()
    FRawDistributionFloat Range;
    UPROPERTY()
    bool bStrengthByDistance = true;
    UPROPERTY()
    FRawDistributionFloat Strength;
    UPROPERTY()
    bool bAffectBaseVelocity = false;
    UPROPERTY()
    TEnumAsByte<EAttractorParticleSelectionMethod> SelectionMethod = EAPSM_Random;
    UPROPERTY()
    bool bRenewSource = false;
    UPROPERTY()
    bool bInheritSourceVel = false;
    UPROPERTY()
    int32 LastSelIndex = 0;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

// UParticleModuleAttractorLine (UE's Particles/Attractor/ParticleModuleAttractorLine.h).
UCLASS()
class UParticleModuleAttractorLine : public UParticleModuleAttractorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAttractorLine(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector EndPoint0;
    UPROPERTY()
    FVector EndPoint1;
    UPROPERTY()
    FRawDistributionFloat Range;
    UPROPERTY()
    FRawDistributionFloat Strength;

    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

// UParticleModuleAttractorPointGravity (UE's Particles/Attractor/ParticleModuleAttractorPointGravity.h).
UCLASS()
class UParticleModuleAttractorPointGravity : public UParticleModuleAttractorBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleAttractorPointGravity(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector Position;
    UPROPERTY()
    float Radius = 0.0f;
    UPROPERTY()
    TObjectPtr<UDistributionFloat> Strength_DEPRECATED = nullptr;
    UPROPERTY()
    FRawDistributionFloat StrengthRaw;

    void PostLoad() override;
};

}
