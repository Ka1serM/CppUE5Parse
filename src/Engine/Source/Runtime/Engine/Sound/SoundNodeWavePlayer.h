#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNodeAssetReferencer.h"

#include "SoundNodeWavePlayer.generated.h"

namespace ue5parse::runtime {

class USoundWave;

UCLASS()
class USoundNodeWavePlayer : public USoundNodeAssetReferencer {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSoftObjectPtr<USoundWave> SoundWaveAssetPtr;
    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SoundWave = nullptr;
    UPROPERTY()
    uint8 bLooping : 1 = 0;
    bool bHasSerializedSoundWave = false;

    void Serialize(FArchive& Ar) override;
};

}
