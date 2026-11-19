#pragma once

#include "Engine/Source/Runtime/Engine/Sound/DialogueTypes.h"

#include "DialogueWave.generated.h"

namespace ue5parse::runtime {

class UDialogueSoundWaveProxy;
class USoundWave;

USTRUCT()
struct FDialogueContextMapping {
    GENERATED_BODY()
    UPROPERTY()
    FDialogueContext Context;
    UPROPERTY()
    TObjectPtr<USoundWave> SoundWave = nullptr;
    UPROPERTY()
    FString LocalizationKeyFormat;
    UPROPERTY()
    TObjectPtr<UDialogueSoundWaveProxy> Proxy = nullptr;
};

UCLASS()
class UDialogueWave : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 bMature : 1 = 0;
    UPROPERTY()
    uint32 bOverride_SubtitleOverride : 1 = 0;
    UPROPERTY()
    FString SpokenText;
    UPROPERTY()
    FString SubtitleOverride;
    UPROPERTY()
    FString VoiceActorDirection;
    UPROPERTY()
    TArray<FDialogueContextMapping> ContextMappings;
    UPROPERTY()
    FGuid LocalizationGUID;

    void Serialize(FArchive& Ar) override;
};

}
