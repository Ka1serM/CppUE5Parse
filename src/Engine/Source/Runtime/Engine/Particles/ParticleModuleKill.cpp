#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleKill.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

UParticleModuleKillBox::UParticleModuleKillBox(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUpdateModule = true;
    bSupported3DDrawMode = true;
    bAxisAlignedAndFixedSize = true;
}

void UParticleModuleKillBox::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    FVector LL = LowerLeftCorner.GetValue(Owner->EmitterTime, Owner->Component);
    FVector UR = UpperRightCorner.GetValue(Owner->EmitterTime, Owner->Component);
    FVector CheckLL = LL.ComponentMin(UR);
    FVector CheckUR = UR.ComponentMax(LL);
    if (bAbsolute == false) {
        CheckLL += Owner->Component->GetComponentLocation();
        CheckUR += Owner->Component->GetComponentLocation();
    }
    FBox CheckBox = FBox(CheckLL, CheckUR);
    BEGIN_UPDATE_LOOP;
    {
        FVector Position = Particle.Location + Owner->PositionOffsetThisTick;
        if (LODLevel->RequiredModule->bUseLocalSpace)
            Position = Owner->Component->GetComponentTransform().TransformVector(Position);
        else if ((bAxisAlignedAndFixedSize == false) && (bAbsolute == false))
            Position = Owner->Component->GetComponentTransform().InverseTransformPosition(Position) + Owner->Component->GetComponentTransform().GetLocation();
        bool bIsInside = CheckBox.IsInside(Position);
        if (bKillInside == bIsInside)
            Owner->KillParticle(i);
    }
    END_UPDATE_LOOP;
}

UParticleModuleKillHeight::UParticleModuleKillHeight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUpdateModule = true;
    bSupported3DDrawMode = true;
}

void UParticleModuleKillHeight::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    UParticleLODLevel* LODLevel = Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
    float CheckHeight = Height.GetValue(Owner->EmitterTime, Owner->Component);
    if (bApplyPSysScale) {
        FVector OwnerScale = Owner->Component->GetComponentTransform().GetScale3D();
        CheckHeight *= OwnerScale.Z;
    }
    if (bAbsolute == false)
        CheckHeight += Owner->Component->GetComponentLocation().Z;
    BEGIN_UPDATE_LOOP;
    {
        FVector Position = Particle.Location;
        if (LODLevel->RequiredModule->bUseLocalSpace)
            Position = Owner->Component->GetComponentTransform().TransformVector(Position);
        if ((bFloor) && (Position.Z < CheckHeight))
            Owner->KillParticle(i);
        else if ((bFloor == false) && (Position.Z > CheckHeight))
            Owner->KillParticle(i);
    }
    END_UPDATE_LOOP;
}

}
