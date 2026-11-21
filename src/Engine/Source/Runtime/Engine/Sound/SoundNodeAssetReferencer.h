#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeAssetReferencer.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class USoundNodeAssetReferencer : public USoundNode {
    GENERATED_BODY()
};

}
