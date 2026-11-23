#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeSoundClass.generated.h"

namespace ue5parse::runtime {

class USoundClass;

UCLASS()
class USoundNodeSoundClass : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USoundClass> SoundClassOverride = nullptr;
};

}
