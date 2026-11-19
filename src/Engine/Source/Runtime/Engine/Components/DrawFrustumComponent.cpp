#include "Engine/Source/Runtime/Engine/Components/DrawFrustumComponent.h"

namespace ue5parse::runtime {

UDrawFrustumComponent::UDrawFrustumComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUseEditorCompositing = true;
    bHiddenInGame = true;
    BodyInstance.CollisionProfileName = FName("NoCollision");
    bGenerateOverlapEvents = false;
}

}
