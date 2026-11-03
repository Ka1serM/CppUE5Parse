#include "ue5parse/runtime/Engine/Particles/ParticleModuleSpawn.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"

// UE's ParticleModules_Spawn.cpp (UE 5.3).

namespace ue5parse::runtime {

UParticleModuleSpawn::UParticleModuleSpawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LODDuplicate = false;
}

float UParticleModuleSpawn::GetMaximumSpawnRate() {
    float MinSpawn, MaxSpawn;
    float MinScale, MaxScale;
    Rate.GetOutRange(MinSpawn, MaxSpawn);
    RateScale.GetOutRange(MinScale, MaxScale);
    return MaxSpawn * MaxScale;
}

// UE averages the keys of constant curve distributions here; the cooked distributions are stubs without keys.
float UParticleModuleSpawn::GetEstimatedSpawnRate() { return GetMaximumSpawnRate(); }

int32 UParticleModuleSpawn::GetMaximumBurstCount() {
    int32 MaxBurst = 0;
    for (const FParticleBurst& Burst : BurstList)
        MaxBurst += Burst.Count;
    return MaxBurst;
}

UParticleModuleSpawnPerUnit::UParticleModuleSpawnPerUnit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = false;
    bUpdateModule = false;
}

uint32 UParticleModuleSpawnPerUnit::RequiredBytesPerInstance() { return sizeof(FParticleSpawnPerUnitInstancePayload); }

bool UParticleModuleSpawnPerUnit::GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, float DeltaTime, int32& Number,
    float& Rate) {
    bool bMoved = false;
    const float ParticlesPerUnit = SpawnPerUnit.GetValue(Owner->EmitterTime, Owner->Component) / UnitScalar;
    Number = 0;
    Rate = 0.0f;
    if (ParticlesPerUnit >= 0.0f) {
        FParticleSpawnPerUnitInstancePayload* SPUPayload = (FParticleSpawnPerUnitInstancePayload*)Owner->GetModuleInstanceData(this);
        const float LeftoverTravel = SPUPayload ? SPUPayload->CurrentDistanceTravelled : 0.0f;
        FVector TravelDirection = Owner->Location - Owner->OldLocation;
        TravelDirection *= FVector(bIgnoreMovementAlongX ? 0.0 : 1.0, bIgnoreMovementAlongY ? 0.0 : 1.0, bIgnoreMovementAlongZ ? 0.0 : 1.0);
        float TravelDistance = float(TravelDirection.Size());
        if (MaxFrameDistance > 0.0f && TravelDistance > MaxFrameDistance) {
            TravelDistance = 0.0f;
            if (SPUPayload)
                SPUPayload->CurrentDistanceTravelled = 0.0f;
        }
        if (TravelDistance > 0.0f) {
            if (TravelDistance > MovementTolerance * UnitScalar)
                bMoved = true;
            const float NewLeftover = (TravelDistance + LeftoverTravel) * ParticlesPerUnit;
            Number = FMath::FloorToInt(NewLeftover);
            const float InvDeltaTime = DeltaTime > 0.0f ? 1.0f / DeltaTime : 0.0f;
            Rate = Number * InvDeltaTime;
            const float NewTravelLeftover = (TravelDistance + LeftoverTravel) - Number * UnitScalar;
            if (SPUPayload)
                SPUPayload->CurrentDistanceTravelled = FMath::Max<float>(0.0f, NewTravelLeftover);
        }
    }
    if (bIgnoreSpawnRateWhenMoving)
        return !bMoved;
    return bProcessSpawnRate;
}

}
