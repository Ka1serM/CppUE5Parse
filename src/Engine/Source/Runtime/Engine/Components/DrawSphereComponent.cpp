#include "Engine/Source/Runtime/Engine/Components/DrawSphereComponent.h"

namespace ue5parse::runtime {

UDrawSphereComponent::UDrawSphereComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("NoCollision");
    bHiddenInGame = true;
    bUseEditorCompositing = true;
    bGenerateOverlapEvents = false;
}

}
