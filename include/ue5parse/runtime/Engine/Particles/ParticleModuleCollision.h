#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleCollision.generated.h"

namespace ue5parse::runtime {

class AActor;
struct FHitResult;

UENUM()
enum EParticleCollisionComplete : int {
    EPCC_Kill,
    EPCC_Freeze,
    EPCC_HaltCollisions,
    EPCC_FreezeTranslation,
    EPCC_FreezeRotation,
    EPCC_FreezeMovement,
    EPCC_MAX,
};

// UParticleModuleCollisionBase (UE's Particles/Collision/ParticleModuleCollisionBase.h).
UCLASS(abstract)
class UParticleModuleCollisionBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleCollision: line checks each particle's move against the world (UE's Particles/Collision/ParticleModuleCollision.h).
UCLASS()
class UParticleModuleCollision : public UParticleModuleCollisionBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleCollision(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector DampingFactor;
    UPROPERTY()
    FRawDistributionVector DampingFactorRotation;
    UPROPERTY()
    FRawDistributionFloat MaxCollisions;
    UPROPERTY()
    TEnumAsByte<EParticleCollisionComplete> CollisionCompletionOption = EPCC_Kill;
    UPROPERTY()
    TArray<TEnumAsByte<EObjectTypeQuery>> CollisionTypes;
    UPROPERTY()
    bool bApplyPhysics = false;
    UPROPERTY()
    bool bIgnoreTriggerVolumes = true;
    UPROPERTY()
    FRawDistributionFloat ParticleMass;
    UPROPERTY()
    float DirScalar = 3.5f;
    UPROPERTY()
    bool bPawnsDoNotDecrementCount = true;
    UPROPERTY()
    bool bOnlyVerticalNormalsDecrementCount = false;
    UPROPERTY()
    float VerticalFudgeFactor = 0.1f;
    UPROPERTY()
    FRawDistributionFloat DelayAmount;
    UPROPERTY()
    bool bDropDetail = true;
    UPROPERTY()
    bool bCollideOnlyIfVisible = true;
    UPROPERTY()
    bool bIgnoreSourceActor = true;
    UPROPERTY()
    float MaxCollisionDistance = 1000.0f;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    uint32 RequiredBytesPerInstance() override;
    uint32 PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) override;
    virtual bool PerformCollisionCheck(FParticleEmitterInstance* Owner, FBaseParticle* InParticle, FHitResult& Hit, AActor* SourceActor, const FVector& End,
                                       const FVector& Start, const FVector& Extent);
};

}
