#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleColor.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

UParticleModuleColor::UParticleModuleColor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = false;
    bCurvesAsColor = true;
    bClampAlpha = true;
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleColor::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        FVector3f ColorVec = FVector3f(StartColor.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream));
        float Alpha = StartAlpha.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        Particle_SetColorFromVector(ColorVec, Alpha, Particle.Color);
        Particle.BaseColor = Particle.Color;
    }
}

UParticleModuleColor_Seeded::UParticleModuleColor_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleColor_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleColorOverLife::UParticleModuleColorOverLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bCurvesAsColor = true;
    bClampAlpha = true;
}

void UParticleModuleColorOverLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector ColorVec = ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
    float fAlpha = AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
    Particle.Color.R = UE_REAL_TO_FLOAT(ColorVec.X);
    Particle.BaseColor.R = UE_REAL_TO_FLOAT(ColorVec.X);
    Particle.Color.G = UE_REAL_TO_FLOAT(ColorVec.Y);
    Particle.BaseColor.G = UE_REAL_TO_FLOAT(ColorVec.Y);
    Particle.Color.B = UE_REAL_TO_FLOAT(ColorVec.Z);
    Particle.BaseColor.B = UE_REAL_TO_FLOAT(ColorVec.Z);
    Particle.Color.A = fAlpha;
    Particle.BaseColor.A = fAlpha;
}

void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if ((Owner == nullptr) || (Owner->ActiveParticles <= 0) || (Owner->ParticleData == nullptr) || (Owner->ParticleIndices == nullptr))
        return;
    const FRawDistribution* FastColorOverLife = ColorOverLife.GetFastRawDistribution();
    const FRawDistribution* FastAlphaOverLife = AlphaOverLife.GetFastRawDistribution();
    if (FastColorOverLife && FastAlphaOverLife) {
        BEGIN_UPDATE_LOOP;
        {
            FastColorOverLife->GetValue3None(Particle.RelativeTime, &Particle.Color.R);
            FastAlphaOverLife->GetValue1None(Particle.RelativeTime, &Particle.Color.A);
        }
        END_UPDATE_LOOP;
    } else {
        FVector ColorVec;
        float fAlpha;
        BEGIN_UPDATE_LOOP;
        {
            ColorVec = ColorOverLife.GetValue(Particle.RelativeTime, Owner->Component);
            fAlpha = AlphaOverLife.GetValue(Particle.RelativeTime, Owner->Component);
            Particle.Color.R = UE_REAL_TO_FLOAT(ColorVec.X);
            Particle.Color.G = UE_REAL_TO_FLOAT(ColorVec.Y);
            Particle.Color.B = UE_REAL_TO_FLOAT(ColorVec.Z);
            Particle.Color.A = fAlpha;
        }
        END_UPDATE_LOOP;
    }
}

UParticleModuleColorScaleOverLife::UParticleModuleColorScaleOverLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bCurvesAsColor = true;
}

void UParticleModuleColorScaleOverLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector ColorVec;
    float fAlpha;
    if (bEmitterTime) {
        ColorVec = ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
        fAlpha = AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
    } else {
        ColorVec = ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
        fAlpha = AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
    }
    Particle.Color.R *= UE_REAL_TO_FLOAT(ColorVec.X);
    Particle.Color.G *= UE_REAL_TO_FLOAT(ColorVec.Y);
    Particle.Color.B *= UE_REAL_TO_FLOAT(ColorVec.Z);
    Particle.Color.A *= fAlpha;
}

void UParticleModuleColorScaleOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    const FRawDistribution* FastColorScaleOverLife = ColorScaleOverLife.GetFastRawDistribution();
    const FRawDistribution* FastAlphaScaleOverLife = AlphaScaleOverLife.GetFastRawDistribution();
    FVector3f ColorVec;
    float fAlpha;
    if (FastColorScaleOverLife && FastAlphaScaleOverLife) {
        if (bEmitterTime) {
            BEGIN_UPDATE_LOOP;
            {
                FastColorScaleOverLife->GetValue3None(Owner->EmitterTime, &ColorVec.X);
                FastAlphaScaleOverLife->GetValue1None(Owner->EmitterTime, &fAlpha);
                Particle.Color.R *= ColorVec.X;
                Particle.Color.G *= ColorVec.Y;
                Particle.Color.B *= ColorVec.Z;
                Particle.Color.A *= fAlpha;
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                FastColorScaleOverLife->GetValue3None(Particle.RelativeTime, &ColorVec.X);
                FastAlphaScaleOverLife->GetValue1None(Particle.RelativeTime, &fAlpha);
                Particle.Color.R *= ColorVec.X;
                Particle.Color.G *= ColorVec.Y;
                Particle.Color.B *= ColorVec.Z;
                Particle.Color.A *= fAlpha;
            }
            END_UPDATE_LOOP;
        }
    } else if (bEmitterTime) {
        BEGIN_UPDATE_LOOP;
        ColorVec = FVector3f(ColorScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component));
        fAlpha = AlphaScaleOverLife.GetValue(Owner->EmitterTime, Owner->Component);
        Particle.Color.R *= ColorVec.X;
        Particle.Color.G *= ColorVec.Y;
        Particle.Color.B *= ColorVec.Z;
        Particle.Color.A *= fAlpha;
        END_UPDATE_LOOP;
    } else {
        BEGIN_UPDATE_LOOP;
        ColorVec = FVector3f(ColorScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component));
        fAlpha = AlphaScaleOverLife.GetValue(Particle.RelativeTime, Owner->Component);
        Particle.Color.R *= ColorVec.X;
        Particle.Color.G *= ColorVec.Y;
        Particle.Color.B *= ColorVec.Z;
        Particle.Color.A *= fAlpha;
        END_UPDATE_LOOP;
    }
}

}
