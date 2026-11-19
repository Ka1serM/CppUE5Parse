#include "Engine/Source/Runtime/Engine/Components/ShapeComponent.h"

namespace ue5parse::runtime {

UShapeComponent::UShapeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("OverlapAllDynamic");
    bHiddenInGame = true;
    bCastDynamicShadow = false;
    bExcludeFromLightAttachmentGroup = true;
    bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
    bCanEverAffectNavigation = true;
}

}
