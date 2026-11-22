#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

#include "SQEXSEADSound.generated.h"

namespace ue5parse::runtime {

//! Reads the flags of the SEAD data and skips the header that these flags give before the bulk data.
bool SerializeSQEXSEADData(FArchive& Ar, int32& Flags, FByteBulkData& Data, int32 HeaderSizeOfFlags17);

//! The SEAD audio of Square Enix follows the sound wave data when the bool before it is set.
UCLASS()
class USQEXSEADSound : public USoundWave {
    GENERATED_BODY()
public:
    explicit USQEXSEADSound(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    int32 SQEXFlags = 0;
    FByteBulkData SQEXSoundData;
};

//! Dragon Quest XI always saves the SEAD bank data; other games save a bool before it.
UCLASS()
class USQEXSEADSoundBank : public UObject {
    GENERATED_BODY()
public:
    explicit USQEXSEADSoundBank(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    int32 SQEXFlags = 0;
    FByteBulkData SQEXSoundBankData;
};

}
