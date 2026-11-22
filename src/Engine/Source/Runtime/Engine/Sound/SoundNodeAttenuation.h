#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundAttenuation.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeAttenuation.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeAttenuation : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;
    UPROPERTY()
    FSoundAttenuationSettings AttenuationOverrides;
    UPROPERTY()
    uint32 bOverrideAttenuation : 1 = 0;
};

}
