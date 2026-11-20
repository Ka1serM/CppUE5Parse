#include "Engine/Source/Runtime/Engine/Components/ArrowComponent.h"

namespace ue5parse::runtime {

UArrowComponent::UArrowComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("NoCollision");
    bHiddenInGame = true;
    bUseEditorCompositing = true;
    bGenerateOverlapEvents = false;
}

}
