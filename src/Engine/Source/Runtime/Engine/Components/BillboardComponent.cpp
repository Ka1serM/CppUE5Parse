#include "Engine/Source/Runtime/Engine/Components/BillboardComponent.h"

namespace ue5parse::runtime {

UBillboardComponent::UBillboardComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("NoCollision");
    SetUsingAbsoluteScale(true);
    bHiddenInGame = true;
    bGenerateOverlapEvents = false;
    bUseEditorCompositing = true;
    bExcludeFromLightAttachmentGroup = true;
}

}
