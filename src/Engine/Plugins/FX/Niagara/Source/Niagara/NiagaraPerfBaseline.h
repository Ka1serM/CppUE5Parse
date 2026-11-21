#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NiagaraPerfBaseline.generated.h"

namespace ue5parse::runtime {

class UTextRenderComponent;

UCLASS()
class ANiagaraPerfBaselineActor : public AActor {
    GENERATED_BODY()
public:
    explicit ANiagaraPerfBaselineActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UObject> Controller = nullptr;
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> Label = nullptr;
};

class ANiagaraPerfBaselineActor;
class UNiagaraEffectType;
class UNiagaraSystem;

UCLASS()
class UNiagaraBaselineController : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraBaselineController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    float TestDuration = 5.0f;
    UPROPERTY()
    TObjectPtr<UNiagaraEffectType> EffectType = nullptr;
    UPROPERTY()
    TObjectPtr<ANiagaraPerfBaselineActor> Owner = nullptr;
    UPROPERTY()
    TSoftObjectPtr<UNiagaraSystem> System;
};

USTRUCT()
struct FNiagaraPerfBaselineStats {
    GENERATED_BODY()

    UPROPERTY()
    float PerInstanceAvg_GT = 0.0f;
    UPROPERTY()
    float PerInstanceAvg_RT = 0.0f;
    UPROPERTY()
    float PerInstanceMax_GT = 0.0f;
    UPROPERTY()
    float PerInstanceMax_RT = 0.0f;
};

}
