#include "Engine/Source/Runtime/UMG/Components/WidgetComponent.h"

namespace ue5parse::runtime {

UWidgetComponent::UWidgetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = true;
}

}
