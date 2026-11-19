#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"

namespace ue5parse::runtime {

UCapsuleComponent::UCapsuleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUseEditorCompositing = true;
}

}
