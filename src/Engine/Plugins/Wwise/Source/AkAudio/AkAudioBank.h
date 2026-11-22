#pragma once

#include "Engine/Plugins/Wwise/Source/AkAudio/AkAudioEvent.h"

#include "AkAudioBank.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAkAudioBank : public UAkAudioType {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

private:
    void SerializeRawProperty(FArchive& Ar, int32 SchemaIndex);
};

}
