#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "DialogueTypes.generated.h"

namespace ue5parse::runtime {

class UDialogueVoice;
class UDialogueWave;

namespace EGrammaticalGender {

UENUM()
enum Type : uint8 { Neuter, Masculine, Feminine, Mixed };

}

namespace EGrammaticalNumber {

UENUM()
enum Type : uint8 { Singular, Plural };

}

USTRUCT()
struct FDialogueContext {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UDialogueVoice> Speaker = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UDialogueVoice>> Targets;
};

USTRUCT()
struct FDialogueWaveParameter {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UDialogueWave> DialogueWave = nullptr;
    UPROPERTY()
    FDialogueContext Context;
};

}
