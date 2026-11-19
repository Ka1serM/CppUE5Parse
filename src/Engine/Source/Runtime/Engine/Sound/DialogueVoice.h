#pragma once

#include "Engine/Source/Runtime/Engine/Sound/DialogueTypes.h"

#include "DialogueVoice.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDialogueVoice : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EGrammaticalGender::Type> Gender = EGrammaticalGender::Neuter;
    UPROPERTY()
    TEnumAsByte<EGrammaticalNumber::Type> Plurality = EGrammaticalNumber::Singular;
    UPROPERTY()
    FGuid LocalizationGUID;
};

}
