#include "ue5parse/runtime/Engine/Particles/ParticleModuleSubUV.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"

// UE's s_ParticleModules.cpp (UE 5.3).

namespace ue5parse::runtime {

struct FSubUVMovieParticlePayload {
    float Time;
};

UParticleModuleSubUV::UParticleModuleSubUV(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleSubUV::PostLoad() {
    Super::PostLoad();
    if (Animation)
        Animation->ConditionalPostLoad();
}

void UParticleModuleSubUV::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    EParticleSubUVInterpMethod InterpMethod = (EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
    const int32 PayloadOffset = Owner->SubUVDataOffset;
    if ((InterpMethod == PSUVIM_None) || (PayloadOffset == 0))
        return;
    if (!LODLevel->TypeDataModule || LODLevel->TypeDataModule->SupportsSubUV()) {
        SPAWN_INIT;
        {
            int32 TempOffset = CurrentOffset;
            CurrentOffset = PayloadOffset;
            PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload);
            CurrentOffset = TempOffset;
            SubUVPayload.ImageIndex = DetermineImageIndex(Owner, Offset, &Particle, InterpMethod, SubUVPayload, SpawnTime);
        }
    }
}

void UParticleModuleSubUV::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    EParticleSubUVInterpMethod InterpMethod = (EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
    const int32 PayloadOffset = Owner->SubUVDataOffset;
    if ((InterpMethod == PSUVIM_None) || (PayloadOffset == 0))
        return;
    if ((InterpMethod == PSUVIM_Random) || (InterpMethod == PSUVIM_Random_Blend)) {
        if (LODLevel->RequiredModule->RandomImageChanges == 0)
            return;
    }
    if (!LODLevel->TypeDataModule || LODLevel->TypeDataModule->SupportsSubUV()) {
        BEGIN_UPDATE_LOOP;
        if (Particle.RelativeTime > 1.0f)
            CONTINUE_UPDATE_LOOP;
        int32 TempOffset = CurrentOffset;
        CurrentOffset = PayloadOffset;
        PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload);
        CurrentOffset = TempOffset;
        SubUVPayload.ImageIndex = DetermineImageIndex(Owner, Offset, &Particle, InterpMethod, SubUVPayload, DeltaTime);
        END_UPDATE_LOOP;
    }
}

float UParticleModuleSubUV::DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle,
    EParticleSubUVInterpMethod InterpMethod, FFullSubUVPayload& SubUVPayload, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    USubUVAnimation* SubUVAnimation = Owner->SpriteTemplate->SubUVAnimation;
    const int32 TotalSubImages = SubUVAnimation ? SubUVAnimation->SubImages_Horizontal * SubUVAnimation->SubImages_Vertical
                                                : LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
    float ImageIndex = SubUVPayload.ImageIndex;
    if ((InterpMethod == PSUVIM_Linear) || (InterpMethod == PSUVIM_Linear_Blend)) {
        if (bUseRealTime == false) {
            ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime, Owner->Component);
        } else {
            UWorld* World = Owner->Component->GetWorld();
            if ((World != nullptr) && (World->GetWorldSettings() != nullptr))
                ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime / World->GetWorldSettings()->GetEffectiveTimeDilation(), Owner->Component);
            else
                ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime, Owner->Component);
        }
        if (InterpMethod == PSUVIM_Linear)
            ImageIndex = FMath::TruncToFloat(ImageIndex);
    } else if ((InterpMethod == PSUVIM_Random) || (InterpMethod == PSUVIM_Random_Blend)) {
        if ((LODLevel->RequiredModule->RandomImageTime == 0.0f) ||
            ((Particle->RelativeTime - SubUVPayload.RandomImageTime) > LODLevel->RequiredModule->RandomImageTime) ||
            (SubUVPayload.RandomImageTime == 0.0f)) {
            ImageIndex = GetRandomStream(Owner).RandHelper(TotalSubImages);
            SubUVPayload.RandomImageTime = Particle->RelativeTime;
        }
        if (InterpMethod == PSUVIM_Random)
            ImageIndex = FMath::TruncToFloat(ImageIndex);
    } else {
        ImageIndex = 0;
    }
    return ImageIndex;
}

UParticleModuleSubUVMovie::UParticleModuleSubUVMovie(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    StartingFrame = 1;
}

void UParticleModuleSubUVMovie::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    int32 SubUVDataOffset = Owner->SubUVDataOffset;
    EParticleSubUVInterpMethod InterpMethod = (EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
    if ((InterpMethod == PSUVIM_None) || (SubUVDataOffset == 0))
        return;
    if ((InterpMethod != PSUVIM_Linear) && (InterpMethod != PSUVIM_Linear_Blend))
        return UParticleModuleSubUV::Spawn(Owner, Offset, SpawnTime, ParticleBase);
    if (!LODLevel->TypeDataModule || LODLevel->TypeDataModule->SupportsSubUV()) {
        USubUVAnimation* SubUVAnimation = Owner->SpriteTemplate->SubUVAnimation;
        int32 TotalSubImages = SubUVAnimation ? SubUVAnimation->SubImages_Horizontal * SubUVAnimation->SubImages_Vertical
                                              : LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;
        if (TotalSubImages == 0)
            TotalSubImages = 1;
        SPAWN_INIT;
        {
            int32 TempOffset = CurrentOffset;
            CurrentOffset = SubUVDataOffset;
            PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload);
            CurrentOffset = TempOffset;
            const float UserSetFrameRate = FrameRate.GetValue(bUseEmitterTime ? Owner->EmitterTime : Particle.RelativeTime, Owner->Component);
            PARTICLE_ELEMENT(FSubUVMovieParticlePayload, MoviePayload);
            MoviePayload.Time = 0.0f;
            if (StartingFrame > 1) {
                MoviePayload.Time = FMath::Clamp<float>(StartingFrame, 0, TotalSubImages - 1);
            } else if (StartingFrame == 0) {
                FRandomStream& RandomStream = GetRandomStream(Owner);
                MoviePayload.Time = FMath::TruncToFloat(RandomStream.FRand() * (TotalSubImages - 1));
            }
            SubUVPayload.ImageIndex = MoviePayload.Time * UserSetFrameRate;
        }
    }
}

uint32 UParticleModuleSubUVMovie::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FSubUVMovieParticlePayload);
}

float UParticleModuleSubUVMovie::DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle,
    EParticleSubUVInterpMethod InterpMethod, FFullSubUVPayload& SubUVPayload, float DeltaTime) {
    FSubUVMovieParticlePayload& MoviePayload = *((FSubUVMovieParticlePayload*)((uint8*)Particle + Offset));
    float UserSetFrameRate = FrameRate.GetValue(bUseEmitterTime ? Owner->EmitterTime : Particle->RelativeTime, Owner->Component);
    if (bUseRealTime == false) {
        MoviePayload.Time += DeltaTime;
    } else {
        UWorld* World = Owner->Component->GetWorld();
        if ((World != nullptr) && (World->GetWorldSettings() != nullptr))
            MoviePayload.Time += DeltaTime / World->GetWorldSettings()->GetEffectiveTimeDilation();
        else
            MoviePayload.Time += DeltaTime;
    }
    float ImageIndex = MoviePayload.Time * UserSetFrameRate;
    if (InterpMethod != PSUVIM_Linear_Blend)
        ImageIndex = FMath::TruncToFloat(ImageIndex);
    return ImageIndex;
}

}
