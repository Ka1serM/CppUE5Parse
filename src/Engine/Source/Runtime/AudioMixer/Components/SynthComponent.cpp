#include "Engine/Source/Runtime/AudioMixer/Components/SynthComponent.h"

namespace ue5parse::runtime {

USynthComponent::USynthComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = false;
    bUseAttachParentBound = true;
}

}
