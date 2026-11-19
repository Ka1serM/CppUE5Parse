#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundAttenuation.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundBase.h"

#include "SoundCue.generated.h"

namespace ue5parse::runtime {

class UEdGraph;
class USoundNode;

UCLASS()
class USoundCue : public USoundBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint8 bOverrideAttenuation : 1 = 0;
    UPROPERTY()
    uint8 bExcludeFromRandomNodeBranchCulling : 1 = 0;
    UPROPERTY()
    uint8 bHasPlayWhenSilent : 1 = 0;
    UPROPERTY()
    uint8 bPrimeOnLoad : 1 = 0;
    UPROPERTY()
    TObjectPtr<USoundNode> FirstNode = nullptr;
    UPROPERTY()
    float VolumeMultiplier = 0.75f;
    UPROPERTY()
    float PitchMultiplier = 1.0f;
    UPROPERTY()
    FSoundAttenuationSettings AttenuationOverrides;
    UPROPERTY()
    float SubtitlePriority = 10000.0f;
    UPROPERTY()
    TArray<TObjectPtr<USoundNode>> AllNodes;
    UPROPERTY()
    TObjectPtr<UEdGraph> SoundCueGraph = nullptr;
    UPROPERTY()
    int32 CookedQualityIndex = INDEX_NONE;

    void Serialize(FArchive& Ar) override;
};

}
