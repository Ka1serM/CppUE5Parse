#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse::runtime {

UBrushComponent::UBrushComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bHiddenInGame = true;
    AlwaysLoadOnClient = false;
    AlwaysLoadOnServer = false;
    bUseAsOccluder = true;
    bUseEditorCompositing = true;
    bCanEverAffectNavigation = true;
}

//! UE 4.9 and newer run this only in the editor (WITH_EDITOR), which loads the reference worlds.
void UBrushComponent::PostLoad() {
    Super::PostLoad();
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_9 || GetOwner() == nullptr)
        return;
    AddRelativeLocation(GetComponentTransform().TransformVector(-PrePivot_DEPRECATED));
    PrePivot_DEPRECATED = FVector::ZeroVector;
}

}
