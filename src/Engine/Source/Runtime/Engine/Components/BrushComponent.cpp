#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

UBrushComponent::UBrushComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bHiddenInGame = true;
    AlwaysLoadOnClient = false;
    AlwaysLoadOnServer = false;
    bUseAsOccluder = true;
    bUseEditorCompositing = true;
    bCanEverAffectNavigation = true;
}

}
