#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLocation.generated.h"

namespace ue5parse::runtime {

// UParticleModuleLocationBase (UE's Particles/Location/ParticleModuleLocationBase.h).
UCLASS(abstract)
class UParticleModuleLocationBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleLocation (UE's Particles/Location/ParticleModuleLocation.h).
UCLASS()
class UParticleModuleLocation : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocation(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartLocation;
    UPROPERTY()
    float DistributeOverNPoints = 0.0f;
    UPROPERTY()
    float DistributeThreshold = 0.0f;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleLocation_Seeded (UE's Particles/Location/ParticleModuleLocation_Seeded.h).
UCLASS()
class UParticleModuleLocation_Seeded : public UParticleModuleLocation {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocation_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModuleLocationWorldOffset (UE's Particles/Location/ParticleModuleLocationWorldOffset.h).
UCLASS()
class UParticleModuleLocationWorldOffset : public UParticleModuleLocation {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationWorldOffset(const FObjectInitializer& ObjectInitializer);

    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase) override;
};

// UParticleModuleLocationWorldOffset_Seeded (UE's Particles/Location/ParticleModuleLocationWorldOffset_Seeded.h).
UCLASS()
class UParticleModuleLocationWorldOffset_Seeded : public UParticleModuleLocationWorldOffset {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationWorldOffset_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModuleLocationDirect (UE's Particles/Location/ParticleModuleLocationDirect.h).
UCLASS()
class UParticleModuleLocationDirect : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationDirect(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector Location;
    UPROPERTY()
    FRawDistributionVector LocationOffset;
    UPROPERTY()
    FRawDistributionVector ScaleFactor;
    UPROPERTY()
    FRawDistributionVector Direction;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
};

UENUM()
enum ELocationEmitterSelectionMethod : int32 { ELESM_Random, ELESM_Sequential, ELESM_MAX };

// UParticleModuleLocationEmitter (UE's Particles/Location/ParticleModuleLocationEmitter.h).
UCLASS()
class UParticleModuleLocationEmitter : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationEmitter(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName EmitterName = NAME_None;
    UPROPERTY()
    TEnumAsByte<ELocationEmitterSelectionMethod> SelectionMethod = ELESM_Random;
    UPROPERTY()
    bool InheritSourceVelocity = false;
    UPROPERTY()
    float InheritSourceVelocityScale = 1.0f;
    UPROPERTY()
    bool bInheritSourceRotation = false;
    UPROPERTY()
    float InheritSourceRotationScale = 1.0f;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    uint32 RequiredBytesPerInstance() override;
};

// UParticleModuleLocationEmitterDirect (UE's Particles/Location/ParticleModuleLocationEmitterDirect.h).
UCLASS()
class UParticleModuleLocationEmitterDirect : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationEmitterDirect(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FName EmitterName = NAME_None;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
};

// UParticleModuleLocationPrimitiveBase (UE's Particles/Location/ParticleModuleLocationPrimitiveBase.h).
UCLASS(abstract)
class UParticleModuleLocationPrimitiveBase : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveBase(const FObjectInitializer& ObjectInitializer);

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
    UPROPERTY()
    bool SurfaceOnly = false;
    UPROPERTY()
    bool Velocity = false;
    UPROPERTY()
    FRawDistributionFloat VelocityScale;
    UPROPERTY()
    FRawDistributionVector StartLocation;

    virtual void DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& vUnitDir, FRandomStream* InRandomStream);
};

// UParticleModuleLocationPrimitiveTriangle (UE's Particles/Location/ParticleModuleLocationPrimitiveTriangle.h).
UCLASS()
class UParticleModuleLocationPrimitiveTriangle : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveTriangle(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionVector StartOffset;
    UPROPERTY()
    FRawDistributionFloat Height;
    UPROPERTY()
    FRawDistributionFloat Angle;
    UPROPERTY()
    FRawDistributionFloat Thickness;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleLocationPrimitiveSphere (UE's Particles/Location/ParticleModuleLocationPrimitiveSphere.h).
UCLASS()
class UParticleModuleLocationPrimitiveSphere : public UParticleModuleLocationPrimitiveBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveSphere(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FRawDistributionFloat StartRadius;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleLocationPrimitiveSphere_Seeded (UE's Particles/Location/ParticleModuleLocationPrimitiveSphere_Seeded.h).
UCLASS()
class UParticleModuleLocationPrimitiveSphere_Seeded : public UParticleModuleLocationPrimitiveSphere {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveSphere_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

UENUM()
enum CylinderHeightAxis : int32 { PMLPC_HEIGHTAXIS_X, PMLPC_HEIGHTAXIS_Y, PMLPC_HEIGHTAXIS_Z, PMLPC_HEIGHTAXIS_MAX };

// UParticleModuleLocationPrimitiveCylinder (UE's Particles/Location/ParticleModuleLocationPrimitiveCylinder.h).
UCLASS()
class UParticleModuleLocationPrimitiveCylinder : public UParticleModuleLocationPrimitiveBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveCylinder(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool RadialVelocity = true;
    UPROPERTY()
    FRawDistributionFloat StartRadius;
    UPROPERTY()
    FRawDistributionFloat StartHeight;
    UPROPERTY()
    TEnumAsByte<CylinderHeightAxis> HeightAxis = PMLPC_HEIGHTAXIS_Z;

    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};

// UParticleModuleLocationPrimitiveCylinder_Seeded (UE's Particles/Location/ParticleModuleLocationPrimitiveCylinder_Seeded.h).
UCLASS()
class UParticleModuleLocationPrimitiveCylinder_Seeded : public UParticleModuleLocationPrimitiveCylinder {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationPrimitiveCylinder_Seeded(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FParticleRandomSeedInfo RandomSeedInfo;

    FParticleRandomSeedInfo* GetRandomSeedInfo() override { return &RandomSeedInfo; }
    void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
};

// UParticleModulePivotOffset: moves the pivot of the sprites; the emitter adds it to its PivotOffset (UE's
// Particles/Location/ParticleModulePivotOffset.h).
UCLASS()
class UParticleModulePivotOffset : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModulePivotOffset(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector2D PivotOffset = FVector2D(0, 0);
};

}
