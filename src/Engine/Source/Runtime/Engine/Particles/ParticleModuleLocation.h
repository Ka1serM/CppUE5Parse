#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleLocation.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UParticleModuleLocationBase : public UParticleModule {
    GENERATED_BODY()
};

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

UCLASS()
class UParticleModuleLocationWorldOffset : public UParticleModuleLocation {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationWorldOffset(const FObjectInitializer& ObjectInitializer);

    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FRandomStream* InRandomStream, FBaseParticle* ParticleBase) override;
};

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

UCLASS()
class UParticleModulePivotOffset : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModulePivotOffset(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVector2D PivotOffset = FVector2D(0, 0);
};

}
