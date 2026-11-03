#include "ue5parse/runtime/Engine/Particles/ParticleModuleAttractor.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"

// UE's s_ParticleModules.cpp (UE 5.3).

namespace ue5parse::runtime {

UParticleModuleAttractorPointGravity::UParticleModuleAttractorPointGravity(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSupported3DDrawMode = true;
}

void UParticleModuleAttractorPointGravity::PostLoad() {
    Super::PostLoad();
}

UParticleModuleAttractorLine::UParticleModuleAttractorLine(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUpdateModule = true;
    bSupported3DDrawMode = true;
}

void UParticleModuleAttractorLine::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    FVector Line = EndPoint1 - EndPoint0;
    if (Line.SizeSquared() == 0.0f)
        Line = FVector(UE_SMALL_NUMBER, UE_SMALL_NUMBER, UE_SMALL_NUMBER);
    FVector LineNorm = Line;
    LineNorm.Normalize();
    BEGIN_UPDATE_LOOP;
    FVector AdjustedLocation = Particle.Location - Owner->Component->GetComponentLocation();
    FVector EP02Particle = AdjustedLocation - EndPoint0;
    FVector ProjectedParticle = Line * (Line | EP02Particle) / (Line | Line);
    float VRatioX = 0.0f;
    float VRatioY = 0.0f;
    float VRatioZ = 0.0f;
    if (Line.X)
        VRatioX = (ProjectedParticle.X - EndPoint0.X) / Line.X;
    if (Line.Y)
        VRatioY = (ProjectedParticle.Y - EndPoint0.Y) / Line.Y;
    if (Line.Z)
        VRatioZ = (ProjectedParticle.Z - EndPoint0.Z) / Line.Z;
    bool bProcess = false;
    float fRatio = 0.0f;
    if (VRatioX || VRatioY || VRatioZ) {
        if (VRatioX)
            fRatio = VRatioX;
        else if (VRatioY)
            fRatio = VRatioY;
        else if (VRatioZ)
            fRatio = VRatioZ;
    }
    if ((fRatio >= 0.0f) && (fRatio <= 1.0f))
        bProcess = true;
    if (bProcess) {
        float AttractorRange = Range.GetValue(fRatio, Owner->Component);
        FVector LineToPoint = AdjustedLocation - ProjectedParticle;
        float Distance = LineToPoint.Size();
        if ((AttractorRange > 0) && (Distance <= AttractorRange)) {
            float AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange, Owner->Component);
            FVector Direction = LineToPoint ^ Line;
            Particle.Velocity += FVector3f(Direction) * AttractorStrength * DeltaTime;
        }
    }
    END_UPDATE_LOOP;
}

UParticleModuleAttractorParticle::UParticleModuleAttractorParticle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bStrengthByDistance = true;
    bAffectBaseVelocity = false;
    SelectionMethod = EAPSM_Random;
    bRenewSource = false;
    LastSelIndex = 0;
    EmitterName = NAME_None;
}

void UParticleModuleAttractorParticle::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    FParticleEmitterInstance* AttractorEmitterInst = nullptr;
    if (EmitterName != NAME_None) {
        for (int32 ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++) {
            FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances[ii];
            if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName)) {
                AttractorEmitterInst = pkEmitInst;
                break;
            }
        }
    }
    if (AttractorEmitterInst == nullptr)
        return;
    SPAWN_INIT {
        PARTICLE_ELEMENT(FAttractorParticlePayload, Data);
        FBaseParticle* Source = AttractorEmitterInst->GetParticle(LastSelIndex);
        if (Source == nullptr) {
            switch (SelectionMethod) {
            case EAPSM_Random:
                LastSelIndex = GetRandomStream(Owner).RandHelper(AttractorEmitterInst->ActiveParticles);
                Data.SourceIndex = LastSelIndex;
                break;
            case EAPSM_Sequential: {
                for (int32 ui = 0; ui < AttractorEmitterInst->ActiveParticles; ui++) {
                    Source = AttractorEmitterInst->GetParticle(ui);
                    if (Source) {
                        LastSelIndex = ui;
                        Data.SourceIndex = LastSelIndex;
                        break;
                    }
                }
            } break;
            }
            Data.SourcePointer = (uint32)(PTRINT)Source;
            if (Source)
                Data.SourceVelocity = Source->Velocity;
        } else {
            Data.SourceIndex = LastSelIndex++;
        }
    }
}

void UParticleModuleAttractorParticle::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    FParticleEmitterInstance* AttractorEmitterInst = nullptr;
    if (EmitterName != NAME_None) {
        for (int32 ii = 0; ii < Owner->Component->EmitterInstances.Num(); ii++) {
            FParticleEmitterInstance* pkEmitInst = Owner->Component->EmitterInstances[ii];
            if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == EmitterName)) {
                AttractorEmitterInst = pkEmitInst;
                break;
            }
        }
    }
    if (AttractorEmitterInst == nullptr)
        return;
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    UParticleLODLevel* SrcLODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(AttractorEmitterInst);
    bool bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
    bool bSrcUseLocalSpace = SrcLODLevel->RequiredModule->bUseLocalSpace;
    BEGIN_UPDATE_LOOP;
    {
        PARTICLE_ELEMENT(FAttractorParticlePayload, Data);
        if (Data.SourceIndex == 0xffffffff) {
            if (bInheritSourceVel)
                Particle.Velocity += Data.SourceVelocity;
            CONTINUE_UPDATE_LOOP;
        }
        FBaseParticle* Source = AttractorEmitterInst->GetParticle(Data.SourceIndex);
        if (!Source)
            CONTINUE_UPDATE_LOOP;
        if ((Data.SourcePointer != 0) && (Source != (FBaseParticle*)(PTRINT)(Data.SourcePointer)) && (bRenewSource == false)) {
            Data.SourceIndex = 0xffffffff;
            Data.SourcePointer = 0;
            if (bInheritSourceVel) {
                Particle.Velocity += Data.SourceVelocity;
                Particle.BaseVelocity += Data.SourceVelocity;
            }
            CONTINUE_UPDATE_LOOP;
        }
        float AttractorRange = Range.GetValue(Source->RelativeTime, Owner->Component);
        FVector SrcLocation = Source->Location;
        FVector ParticleLocation = Particle.Location;
        if (bUseLocalSpace != bSrcUseLocalSpace) {
            if (bSrcUseLocalSpace)
                SrcLocation = Owner->Component->GetComponentTransform().TransformVector(SrcLocation);
            if (bUseLocalSpace)
                ParticleLocation = Owner->Component->GetComponentTransform().TransformVector(Particle.Location);
        }
        FVector Dir = SrcLocation - ParticleLocation;
        float Distance = Dir.Size();
        if (Distance <= AttractorRange) {
            float AttractorStrength = 0.0f;
            if (bStrengthByDistance)
                AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange);
            else
                AttractorStrength = Strength.GetValue(Source->RelativeTime, Owner->Component);
            Dir.Normalize();
            Particle.Velocity += FVector3f(Dir) * AttractorStrength * DeltaTime;
            Data.SourceVelocity = Source->Velocity;
            if (bAffectBaseVelocity)
                Particle.BaseVelocity += FVector3f(Dir) * AttractorStrength * DeltaTime;
        }
    }
    END_UPDATE_LOOP;
}

uint32 UParticleModuleAttractorParticle::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FAttractorParticlePayload);
}

UParticleModuleAttractorPoint::UParticleModuleAttractorPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUpdateModule = true;
    StrengthByDistance = true;
    bAffectBaseVelocity = false;
    bOverrideVelocity = false;
    bSupported3DDrawMode = true;
    Positive_X = true;
    Positive_Y = true;
    Positive_Z = true;
    Negative_X = true;
    Negative_Y = true;
    Negative_Z = true;
}

void UParticleModuleAttractorPoint::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleSystemComponent* Component = Owner->Component;
    FVector AttractorPosition = Position.GetValue(Owner->EmitterTime, Component);
    float AttractorRange = Range.GetValue(Owner->EmitterTime, Component);
    FVector Scale = FVector(1.0f, 1.0f, 1.0f);
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    if ((LODLevel->RequiredModule->bUseLocalSpace == false) && (bUseWorldSpacePosition == false)) {
        AttractorPosition = Component->GetComponentTransform().TransformPosition(AttractorPosition);
        Scale *= Component->GetComponentTransform().GetScale3D();
    }
    float ScaleSize = Scale.Size();
    AttractorRange *= ScaleSize;
    FVector MinNormalizedDir(Negative_X ? -1.0f : 0.0f, Negative_Y ? -1.0f : 0.0f, Negative_Z ? -1.0f : 0.0f);
    FVector MaxNormalizedDir(Positive_X ? +1.0f : 0.0f, Positive_Y ? +1.0f : 0.0f, Positive_Z ? +1.0f : 0.0f);
    BEGIN_UPDATE_LOOP;
    FVector Dir = AttractorPosition - Particle.Location;
    float Distance = Dir.Size();
    if (Distance <= AttractorRange) {
        float AttractorStrength = 0.0f;
        if (StrengthByDistance)
            if (AttractorRange == 0.0f)
                AttractorStrength = 0.0f;
            else
                AttractorStrength = Strength.GetValue((AttractorRange - Distance) / AttractorRange, Component);
        else
            AttractorStrength = Strength.GetValue(Owner->EmitterTime, Component);
        if ((LODLevel->RequiredModule->bUseLocalSpace == false) && (bUseWorldSpacePosition == false))
            AttractorStrength *= ScaleSize;
        Dir.Normalize();
        if (AttractorStrength < 0.0f) {
            Dir = -Dir;
            AttractorStrength = -AttractorStrength;
        }
        Dir = ClampVector(Dir, MinNormalizedDir, MaxNormalizedDir);
        Particle.Velocity += FVector3f(Dir) * AttractorStrength * DeltaTime;
        if (bAffectBaseVelocity)
            Particle.BaseVelocity += FVector3f(Dir) * AttractorStrength * DeltaTime;
    }
    END_UPDATE_LOOP;
}

}
