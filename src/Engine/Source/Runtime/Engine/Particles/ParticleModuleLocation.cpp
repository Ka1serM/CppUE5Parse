#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleLocation::UParticleModuleLocation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupported3DDrawMode = true;
    DistributeOverNPoints = 0.0f;
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLocation::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector LocationOffset;
    if ((DistributeOverNPoints != 0.0f) && (DistributeOverNPoints != 1.f)) {
        float RandomNum = InRandomStream->FRand() * FMath::Fractional(Owner->EmitterTime);
        if (RandomNum > DistributeThreshold) {
            LocationOffset = StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
        } else {
            FVector Min, Max;
            StartLocation.GetRange(Min, Max);
            FVector Lerped = FMath::Lerp(
                Min, Max, FMath::TruncToFloat((InRandomStream->FRand() * (DistributeOverNPoints - 1.0f)) + 0.5f) / (DistributeOverNPoints - 1.0f));
            LocationOffset.Set(Lerped.X, Lerped.Y, Lerped.Z);
        }
    } else {
        LocationOffset = StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    }
    LocationOffset = Owner->EmitterToSimulation.TransformVector(LocationOffset);
    Particle.Location += LocationOffset;
}

UParticleModuleLocation_Seeded::UParticleModuleLocation_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLocation_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleLocationWorldOffset::UParticleModuleLocationWorldOffset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

void UParticleModuleLocationWorldOffset::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (LODLevel->RequiredModule->bUseLocalSpace == false) {
        Particle.Location += StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    } else {
        FMatrix InvMat = Owner->Component->GetComponentTransform().ToMatrixWithScale().InverseFast();
        FVector StartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
        Particle.Location += InvMat.TransformVector(StartLoc);
    }
}

UParticleModuleLocationWorldOffset_Seeded::UParticleModuleLocationWorldOffset_Seeded(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLocationWorldOffset_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleLocationDirect::UParticleModuleLocationDirect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleLocationDirect::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if (LODLevel->RequiredModule->bUseLocalSpace) {
        Particle.Location = Location.GetValue(Particle.RelativeTime, Owner->Component);
    } else {
        FVector StartLoc = Location.GetValue(Particle.RelativeTime, Owner->Component);
        StartLoc = Owner->Component->GetComponentTransform().TransformPosition(StartLoc);
        Particle.Location = StartLoc;
    }
    PARTICLE_ELEMENT(FVector, LocOffset);
    LocOffset = LocationOffset.GetValue(Owner->EmitterTime, Owner->Component);
    Particle.Location += LocOffset;
}

void UParticleModuleLocationDirect::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    const FTransform& OwnerTM = Owner->Component->GetComponentTransform();
    BEGIN_UPDATE_LOOP;
    {
        FVector NewLoc;
        UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
        if (LODLevel->RequiredModule->bUseLocalSpace) {
            NewLoc = Location.GetValue(Particle.RelativeTime, Owner->Component);
        } else {
            FVector Loc = Location.GetValue(Particle.RelativeTime, Owner->Component);
            Loc = OwnerTM.TransformPosition(Loc);
            NewLoc = Loc;
        }
        FVector Scale = ScaleFactor.GetValue(Particle.RelativeTime, Owner->Component);
        PARTICLE_ELEMENT(FVector, LocOffset);
        NewLoc += LocOffset;
        FVector Diff = (NewLoc - Particle.Location);
        FVector ScaleDiffA = Diff * Scale.X;
        FVector ScaleDiffB = Diff * (1.0f - Scale.X);
        float InvDeltaTime = (DeltaTime > 0.0f) ? 1.0f / DeltaTime : 0.0f;
        Particle.Velocity = FVector3f(ScaleDiffA) * InvDeltaTime;
        Particle.Location += ScaleDiffB;
    }
    END_UPDATE_LOOP;
}

uint32 UParticleModuleLocationDirect::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FVector);
}

UParticleModuleLocationEmitter::UParticleModuleLocationEmitter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    SelectionMethod = ELESM_Random;
    EmitterName = NAME_None;
    InheritSourceVelocity = false;
    InheritSourceVelocityScale = 1.0f;
    bInheritSourceRotation = false;
    InheritSourceRotationScale = 1.0f;
}

void UParticleModuleLocationEmitter::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    FParticleEmitterInstance* LocationEmitterInst = nullptr;
    if (EmitterName != NAME_None) {
        for (int32 ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++) {
            FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances[ii];
            if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName)) {
                LocationEmitterInst = pkEmitInst;
                break;
            }
        }
    }
    if (LocationEmitterInst == nullptr)
        return;
    bool bSourceIsInLocalSpace = LocationEmitterInst->CurrentLODLevel->RequiredModule->bUseLocalSpace;
    bool bInLocalSpace = Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace;
    FRandomStream& RandomStream = GetRandomStream(Owner);
    SPAWN_INIT;
    {
        int32 Index = 0;
        switch (SelectionMethod) {
        case ELESM_Random: {
            Index = RandomStream.RandHelper(LocationEmitterInst->ActiveParticles);
        } break;
        case ELESM_Sequential: {
            FLocationEmitterInstancePayload* Payload = (FLocationEmitterInstancePayload*)(Owner->GetModuleInstanceData(this));
            if (Payload != nullptr) {
                Index = ++(Payload->LastSelectedIndex);
                if (Index >= LocationEmitterInst->ActiveParticles) {
                    Index = 0;
                    Payload->LastSelectedIndex = Index;
                }
            } else {
            }
        } break;
        }
        FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(Index);
        if (pkParticle) {
            if ((pkParticle->RelativeTime == 0.0f) && (pkParticle->Location == FVector::ZeroVector))
                if (bInLocalSpace == false)
                    Particle.Location = LocationEmitterInst->Component->GetComponentLocation();
                else
                    Particle.Location = FVector::ZeroVector;
            else if (bSourceIsInLocalSpace == bInLocalSpace)
                Particle.Location = pkParticle->Location;
            else if ((bSourceIsInLocalSpace) && (bInLocalSpace == false))
                Particle.Location = LocationEmitterInst->Component->GetComponentTransform().TransformPosition(pkParticle->Location);
            else
                Particle.Location = LocationEmitterInst->Component->GetComponentTransform().InverseTransformPosition(pkParticle->Location);
            if (InheritSourceVelocity) {
                Particle.BaseVelocity += pkParticle->Velocity * InheritSourceVelocityScale;
                Particle.Velocity += pkParticle->Velocity * InheritSourceVelocityScale;
            }
            if (bInheritSourceRotation) {
                Particle.Rotation += pkParticle->Rotation * InheritSourceRotationScale;
                const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
                const int32 SrcMeshRotationOffset = LocationEmitterInst->GetMeshRotationOffset();
                if (MeshRotationOffset && SrcMeshRotationOffset) {
                    FMeshRotationPayloadData* DestPayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                    FMeshRotationPayloadData* SrcPayloadData = (FMeshRotationPayloadData*)((uint8*)pkParticle + SrcMeshRotationOffset);
                    DestPayloadData->Rotation += SrcPayloadData->Rotation;
                    DestPayloadData->InitialOrientation += SrcPayloadData->InitialOrientation;
                }
            }
        }
    }
}

uint32 UParticleModuleLocationEmitter::RequiredBytesPerInstance() {
    return sizeof(FLocationEmitterInstancePayload);
}

UParticleModuleLocationEmitterDirect::UParticleModuleLocationEmitterDirect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    EmitterName = NAME_None;
}

void UParticleModuleLocationEmitterDirect::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    FParticleEmitterInstance* LocationEmitterInst = nullptr;
    if (EmitterName != NAME_None) {
        for (int32 ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++) {
            FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances[ii];
            if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName)) {
                LocationEmitterInst = pkEmitInst;
                break;
            }
        }
    }
    if (LocationEmitterInst == nullptr)
        return;
    SPAWN_INIT;
    int32 Index = Owner->ActiveParticles;
    FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(Index);
    if (pkParticle) {
        Particle.Location = pkParticle->Location;
        Particle.OldLocation = pkParticle->OldLocation;
        Particle.Velocity = pkParticle->Velocity;
        Particle.RelativeTime = pkParticle->RelativeTime;
    }
}

void UParticleModuleLocationEmitterDirect::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    FParticleEmitterInstance* LocationEmitterInst = nullptr;
    if (EmitterName != NAME_None) {
        for (int32 ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++) {
            FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances[ii];
            if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName)) {
                LocationEmitterInst = pkEmitInst;
                break;
            }
        }
    }
    if (LocationEmitterInst == nullptr)
        return;
    BEGIN_UPDATE_LOOP;
    {
        FBaseParticle* pkParticle = LocationEmitterInst->GetParticle(i);
        if (pkParticle) {
            Particle.Location = pkParticle->Location;
            Particle.OldLocation = pkParticle->OldLocation;
            Particle.Velocity = pkParticle->Velocity;
            Particle.RelativeTime = pkParticle->RelativeTime;
        }
    }
    END_UPDATE_LOOP;
}

UParticleModuleLocationPrimitiveBase::UParticleModuleLocationPrimitiveBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    Positive_X = true;
    Positive_Y = true;
    Positive_Z = true;
    Negative_X = true;
    Negative_Y = true;
    Negative_Z = true;
    SurfaceOnly = false;
    Velocity = false;
}

void UParticleModuleLocationPrimitiveBase::DetermineUnitDirection(
    FParticleEmitterInstance* Owner, FVector& vUnitDir, struct FRandomStream* InRandomStream) {
    FVector vRand;
    vRand.X = InRandomStream->GetFraction();
    vRand.Y = InRandomStream->GetFraction();
    vRand.Z = InRandomStream->GetFraction();
    if (Positive_X && Negative_X)
        vUnitDir.X = vRand.X * 2 - 1;
    else if (Positive_X)
        vUnitDir.X = vRand.X;
    else if (Negative_X)
        vUnitDir.X = -vRand.X;
    else
        vUnitDir.X = 0.0f;
    if (Positive_Y && Negative_Y)
        vUnitDir.Y = vRand.Y * 2 - 1;
    else if (Positive_Y)
        vUnitDir.Y = vRand.Y;
    else if (Negative_Y)
        vUnitDir.Y = -vRand.Y;
    else
        vUnitDir.Y = 0.0f;
    if (Positive_Z && Negative_Z)
        vUnitDir.Z = vRand.Z * 2 - 1;
    else if (Positive_Z)
        vUnitDir.Z = vRand.Z;
    else if (Negative_Z)
        vUnitDir.Z = -vRand.Z;
    else
        vUnitDir.Z = 0.0f;
}

UParticleModuleLocationPrimitiveTriangle::UParticleModuleLocationPrimitiveTriangle(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSupported3DDrawMode = true;
    bSpawnModule = true;
}

void UParticleModuleLocationPrimitiveTriangle::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLocationPrimitiveTriangle::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector TriOffset = StartOffset.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    float TriHeight = Height.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    float TriAngle = Angle.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    float TriThickness = Thickness.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    float BaseLength = TriHeight * FMath::Tan(0.5f * TriAngle * UE_PI / 180.0f);
    FVector Corners[3];
    Corners[0] = TriOffset + FVector(+TriHeight * 0.5f, 0.0f, 0.0f);
    Corners[1] = TriOffset + FVector(-TriHeight * 0.5f, +BaseLength, 0.0f);
    Corners[2] = TriOffset + FVector(-TriHeight * 0.5f, -BaseLength, 0.0f);
    float BarycentricCoords[3] = {0};
    float ZPos = 0.0f;
    BarycentricCoords[0] = InRandomStream->GetFraction();
    BarycentricCoords[1] = InRandomStream->GetFraction();
    BarycentricCoords[2] = InRandomStream->GetFraction();
    ZPos = InRandomStream->GetFraction();
    FVector LocationOffset = FVector::ZeroVector;
    float Sum = FMath::Max<float>(UE_KINDA_SMALL_NUMBER, BarycentricCoords[0] + BarycentricCoords[1] + BarycentricCoords[2]);
    for (int32 i = 0; i < 3; i++)
        LocationOffset += (BarycentricCoords[i] / Sum) * Corners[i];
    LocationOffset.Z = ZPos * TriThickness - 0.5f * TriThickness;
    LocationOffset = Owner->EmitterToSimulation.TransformVector(LocationOffset);
    Particle.Location += LocationOffset;
}

UParticleModuleLocationPrimitiveCylinder::UParticleModuleLocationPrimitiveCylinder(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    RadialVelocity = true;
    bSupported3DDrawMode = true;
    HeightAxis = PMLPC_HEIGHTAXIS_Z;
}

void UParticleModuleLocationPrimitiveCylinder::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLocationPrimitiveCylinder::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    int32 RadialIndex0 = 0;
    int32 RadialIndex1 = 1;
    int32 HeightIndex = 2;
    switch (HeightAxis) {
    case PMLPC_HEIGHTAXIS_X:
        RadialIndex0 = 1;
        RadialIndex1 = 2;
        HeightIndex = 0;
        break;
    case PMLPC_HEIGHTAXIS_Y:
        RadialIndex0 = 0;
        RadialIndex1 = 2;
        HeightIndex = 1;
        break;
    case PMLPC_HEIGHTAXIS_Z:
        break;
    }
    FVector vStartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    FVector vOffset(0.0f);
    float fStartRadius = StartRadius.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    float fStartHeight = StartHeight.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream) / 2.0f;
    FVector vUnitDir, vUnitDirTemp;
    bool bFoundValidValue = false;
    int32 NumberOfAttempts = 0;
    float RadiusSquared = fStartRadius * fStartRadius;
    while (!bFoundValidValue) {
        DetermineUnitDirection(Owner, vUnitDirTemp, InRandomStream);
        vUnitDir[RadialIndex0] = vUnitDirTemp[RadialIndex0];
        vUnitDir[RadialIndex1] = vUnitDirTemp[RadialIndex1];
        vUnitDir[HeightIndex] = vUnitDirTemp[HeightIndex];
        FVector2D CheckVal(vUnitDir[RadialIndex0] * fStartRadius, vUnitDir[RadialIndex1] * fStartRadius);
        if (CheckVal.SizeSquared() <= RadiusSquared)
            bFoundValidValue = true;
        else if (NumberOfAttempts >= 50)
            bFoundValidValue = true;
        NumberOfAttempts++;
    }
    FVector vNormalizedDir = vUnitDir;
    vNormalizedDir.Normalize();
    FVector2D vUnitDir2D(vUnitDir[RadialIndex0], vUnitDir[RadialIndex1]);
    FVector2D vNormalizedDir2D = vUnitDir2D.GetSafeNormal();
    vOffset[HeightIndex] = vUnitDir[HeightIndex] * fStartHeight;
    vNormalizedDir[RadialIndex0] = vNormalizedDir2D.X;
    vNormalizedDir[RadialIndex1] = vNormalizedDir2D.Y;
    if (SurfaceOnly) {
        if (FMath::IsNearlyZero(FMath::Abs(vOffset[HeightIndex]) - fStartHeight)) {
            vOffset[RadialIndex0] = vUnitDir[RadialIndex0] * fStartRadius;
            vOffset[RadialIndex1] = vUnitDir[RadialIndex1] * fStartRadius;
        } else {
            vOffset[RadialIndex0] = vNormalizedDir[RadialIndex0] * fStartRadius;
            vOffset[RadialIndex1] = vNormalizedDir[RadialIndex1] * fStartRadius;
        }
    } else {
        vOffset[RadialIndex0] = vUnitDir[RadialIndex0] * fStartRadius;
        vOffset[RadialIndex1] = vUnitDir[RadialIndex1] * fStartRadius;
    }
    FVector vMax;
    vMax[RadialIndex0] = FMath::Abs(vNormalizedDir[RadialIndex0]) * fStartRadius;
    vMax[RadialIndex1] = FMath::Abs(vNormalizedDir[RadialIndex1]) * fStartRadius;
    vMax[HeightIndex] = fStartHeight;
    vOffset[RadialIndex0] = FMath::Clamp<float>(vOffset[RadialIndex0], -vMax[RadialIndex0], vMax[RadialIndex0]);
    vOffset[RadialIndex1] = FMath::Clamp<float>(vOffset[RadialIndex1], -vMax[RadialIndex1], vMax[RadialIndex1]);
    vOffset[HeightIndex] = FMath::Clamp<float>(vOffset[HeightIndex], -vMax[HeightIndex], vMax[HeightIndex]);
    vOffset[RadialIndex0] += vStartLoc[RadialIndex0];
    vOffset[RadialIndex1] += vStartLoc[RadialIndex1];
    vOffset[HeightIndex] += vStartLoc[HeightIndex];
    Particle.Location += Owner->EmitterToSimulation.TransformVector(vOffset);
    if (Velocity) {
        FVector vVelocity;
        vVelocity[RadialIndex0] = vOffset[RadialIndex0] - vStartLoc[RadialIndex0];
        vVelocity[RadialIndex1] = vOffset[RadialIndex1] - vStartLoc[RadialIndex1];
        vVelocity[HeightIndex] = vOffset[HeightIndex] - vStartLoc[HeightIndex];
        if (RadialVelocity)
            vVelocity[HeightIndex] = 0.0f;
        vVelocity *= VelocityScale.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        vVelocity = Owner->EmitterToSimulation.TransformVector(vVelocity);
        Particle.Velocity += FVector3f(vVelocity);
        Particle.BaseVelocity += FVector3f(vVelocity);
    }
}

UParticleModuleLocationPrimitiveCylinder_Seeded::UParticleModuleLocationPrimitiveCylinder_Seeded(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLocationPrimitiveCylinder_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleLocationPrimitiveSphere::UParticleModuleLocationPrimitiveSphere(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSupported3DDrawMode = true;
}

void UParticleModuleLocationPrimitiveSphere::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLocationPrimitiveSphere::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector vStartLoc = StartLocation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    FVector vUnitDir;
    DetermineUnitDirection(Owner, vUnitDir, InRandomStream);
    FVector vNormalizedDir = vUnitDir;
    vNormalizedDir.Normalize();
    if (SurfaceOnly)
        vUnitDir.Normalize();
    float fStartRadius = StartRadius.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    FVector vStartRadius = FVector(fStartRadius);
    FVector vOffset = vUnitDir * vStartRadius;
    FVector vMax;
    vMax.X = FMath::Abs(vNormalizedDir.X) * fStartRadius;
    vMax.Y = FMath::Abs(vNormalizedDir.Y) * fStartRadius;
    vMax.Z = FMath::Abs(vNormalizedDir.Z) * fStartRadius;
    if (Positive_X || Negative_X)
        vOffset.X = FMath::Clamp<float>(vOffset.X, -vMax.X, vMax.X);
    else
        vOffset.X = 0.0f;
    if (Positive_Y || Negative_Y)
        vOffset.Y = FMath::Clamp<float>(vOffset.Y, -vMax.Y, vMax.Y);
    else
        vOffset.Y = 0.0f;
    if (Positive_Z || Negative_Z)
        vOffset.Z = FMath::Clamp<float>(vOffset.Z, -vMax.Z, vMax.Z);
    else
        vOffset.Z = 0.0f;
    vOffset += vStartLoc;
    Particle.Location += Owner->EmitterToSimulation.TransformVector(vOffset);
    if (Velocity) {
        FVector vVelocity = (vOffset - vStartLoc) * VelocityScale.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        vVelocity = Owner->EmitterToSimulation.TransformVector(vVelocity);
        Particle.Velocity += FVector3f(vVelocity);
        Particle.BaseVelocity += FVector3f(vVelocity);
    }
}

UParticleModuleLocationPrimitiveSphere_Seeded::UParticleModuleLocationPrimitiveSphere_Seeded(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLocationPrimitiveSphere_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModulePivotOffset::UParticleModulePivotOffset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = false;
    bUpdateModule = false;
}

}
