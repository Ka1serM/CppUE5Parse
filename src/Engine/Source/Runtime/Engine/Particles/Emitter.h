#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Emitter.generated.h"

namespace ue5parse::runtime {

class UPhysicalMaterial;
class UParticleSystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FParticleSpawnSignature, FName, EventName, float, EmitterTime, FVector, Location, FVector, Velocity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FParticleBurstSignature, FName, EventName, float, EmitterTime, int32, ParticleCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FParticleDeathSignature, FName, EventName, float, EmitterTime, int32, ParticleTime, FVector, Location, FVector,
                                             Velocity, FVector, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams(FParticleCollisionSignature, FName, EventName, float, EmitterTime, int32, ParticleTime, FVector, Location,
                                              FVector, Velocity, FVector, Direction, FVector, Normal, FName, BoneName, UPhysicalMaterial*, PhysMat);

UCLASS()
class AEmitter : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UParticleSystemComponent> ParticleSystemComponent = nullptr;
    UPROPERTY()
    uint8 bDestroyOnSystemFinish : 1 = false;
    UPROPERTY()
    uint8 bPostUpdateTickGroup : 1 = false;
    UPROPERTY()
    uint8 bCurrentlyActive : 1 = false;
    UPROPERTY()
    FParticleSpawnSignature OnParticleSpawn;
    UPROPERTY()
    FParticleBurstSignature OnParticleBurst;
    UPROPERTY()
    FParticleDeathSignature OnParticleDeath;
    UPROPERTY()
    FParticleCollisionSignature OnParticleCollide;

    explicit AEmitter(const FObjectInitializer& ObjectInitializer);
};

}
