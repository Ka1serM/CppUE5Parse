#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleSystem.h"

#include "NiagaraSystem.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraSystem : public UFXSystemAsset {
    GENERATED_BODY()
public:
    explicit UNiagaraSystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
