#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

#include "NiagaraComponent.generated.h"

namespace ue5parse::runtime {

class UNiagaraComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNiagaraSystemFinished, UNiagaraComponent*, PSystem);

UCLASS()
class UNiagaraComponent : public UFXSystemComponent {
    GENERATED_BODY()
public:
    explicit UNiagaraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UNiagaraSystem> Asset = nullptr;
    UPROPERTY()
    FNiagaraUserRedirectionParameterStore OverrideParameters;
    UPROPERTY()
    bool bForceSolo = false;
    UPROPERTY()
    bool bAutoDestroy = false;
    UPROPERTY()
    bool bRenderingEnabled = true;
    UPROPERTY()
    float DesiredAge = 0.0f;
    UPROPERTY()
    float SeekDelta = 1.0f / 30.0f;
    UPROPERTY()
    FOnNiagaraSystemFinished OnSystemFinished;
};

}
