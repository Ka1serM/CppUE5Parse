#pragma once

#include "Engine/Source/Runtime/Engine/Sound/DialogueTypes.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeDialoguePlayer.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeDialoguePlayer : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    FDialogueWaveParameter DialogueWaveParameter;
    UPROPERTY()
    uint32 bLooping : 1 = 0;
};

}
