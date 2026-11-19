#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "WorldPSCPool.generated.h"

namespace ue5parse::runtime {

class UParticleSystem;
class UParticleSystemComponent;
class UWorld;

UENUM()
enum class EPSCPoolMethod : uint8 { None, AutoRelease, ManualRelease, ManualRelease_OnComplete, FreeInPool };

USTRUCT()
struct FPSCPoolElem {
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystemComponent> PSC = nullptr;
    float LastUsedTime = 0;
};

USTRUCT()
struct FPSCPool {
    GENERATED_BODY()

    UPROPERTY(Transient)
    TArray<FPSCPoolElem> FreeElements;

    UParticleSystemComponent* Acquire(UWorld* World, UParticleSystem* Template, EPSCPoolMethod PoolingMethod);
    void Reclaim(UParticleSystemComponent* PSC, float CurrentTimeSeconds);
    void KillUnusedComponents(float KillTime);
};

USTRUCT()
struct FWorldPSCPool {
    GENERATED_BODY()

    UParticleSystemComponent* CreateWorldParticleSystem(UParticleSystem* Template, UWorld* World, EPSCPoolMethod PoolingMethod);
    void ReclaimWorldParticleSystem(UParticleSystemComponent* PSC);

private:
    UPROPERTY()
    TMap<TObjectPtr<UParticleSystem>, FPSCPool> WorldParticleSystemPools;
    float LastParticleSytemPoolCleanTime = 0;
};

}
