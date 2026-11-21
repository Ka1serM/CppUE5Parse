#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/ForceFeedbackAttenuation.h"

#include "ForceFeedbackComponent.generated.h"

namespace ue5parse::runtime {

class UForceFeedbackComponent;
class UForceFeedbackEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnForceFeedbackFinished, UForceFeedbackComponent*, ForceFeedbackComponent);

UCLASS()
class UForceFeedbackComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UForceFeedbackComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UForceFeedbackEffect> ForceFeedbackEffect = nullptr;
    UPROPERTY()
    uint8 bAutoDestroy : 1 = false;
    UPROPERTY()
    uint8 bStopWhenOwnerDestroyed : 1 = false;
    UPROPERTY()
    uint8 bLooping : 1 = true;
    UPROPERTY()
    uint8 bIgnoreTimeDilation : 1 = false;
    UPROPERTY()
    uint8 bOverrideAttenuation : 1 = false;
    UPROPERTY()
    float IntensityMultiplier = 1.0f;
    UPROPERTY()
    TObjectPtr<UForceFeedbackAttenuation> AttenuationSettings = nullptr;
    UPROPERTY()
    FForceFeedbackAttenuationSettings AttenuationOverrides;
    UPROPERTY()
    FOnForceFeedbackFinished OnForceFeedbackFinished;
};

}
