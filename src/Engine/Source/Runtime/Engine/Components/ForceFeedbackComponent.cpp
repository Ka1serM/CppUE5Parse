#include "Engine/Source/Runtime/Engine/Components/ForceFeedbackComponent.h"

namespace ue5parse::runtime {

UForceFeedbackComponent::UForceFeedbackComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
}

}
