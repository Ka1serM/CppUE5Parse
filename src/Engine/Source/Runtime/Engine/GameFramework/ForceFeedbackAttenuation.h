#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Attenuation.h"

#include "ForceFeedbackAttenuation.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FForceFeedbackAttenuationSettings : public FBaseAttenuationSettings {
    GENERATED_BODY()
};

UCLASS()
class UForceFeedbackAttenuation : public UObject {
    GENERATED_BODY()
public:
    explicit UForceFeedbackAttenuation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FForceFeedbackAttenuationSettings Attenuation;
};

}
