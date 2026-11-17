#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_PlaySound.generated.h"

namespace ue5parse::runtime {

class USoundBase;

UCLASS(const)
class UAnimNotify_PlaySound : public UAnimNotify {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USoundBase> Sound = nullptr;
    UPROPERTY()
    float VolumeMultiplier = 1.0f;
    UPROPERTY()
    float PitchMultiplier = 1.0f;
    UPROPERTY()
    bool bFollow = false;
    UPROPERTY()
    FName AttachName;
};

}
