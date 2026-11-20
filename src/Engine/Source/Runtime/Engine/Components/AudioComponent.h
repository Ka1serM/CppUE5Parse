#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "AudioComponent.generated.h"

namespace ue5parse::runtime {

class USoundAttenuation;
class USoundBase;
class USoundClass;
class USoundConcurrency;
class USoundEffectSourcePresetChain;

UCLASS()
class UAudioComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USoundBase> Sound = nullptr;
    UPROPERTY()
    TObjectPtr<USoundClass> SoundClassOverride = nullptr;
    UPROPERTY()
    TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;
    UPROPERTY()
    TObjectPtr<USoundConcurrency> ConcurrencySettings = nullptr;
    UPROPERTY()
    TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain = nullptr;

    explicit UAudioComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
