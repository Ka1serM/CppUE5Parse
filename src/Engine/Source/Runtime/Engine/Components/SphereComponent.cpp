#include "Engine/Source/Runtime/Engine/Components/SphereComponent.h"

namespace ue5parse::runtime {

USphereComponent::USphereComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ShapeColor = FColor(255, 0, 0, 255);
    bUseEditorCompositing = true;
}

}
