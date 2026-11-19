#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"

namespace ue5parse::runtime {

UBoxComponent::UBoxComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUseEditorCompositing = true;
}

}
