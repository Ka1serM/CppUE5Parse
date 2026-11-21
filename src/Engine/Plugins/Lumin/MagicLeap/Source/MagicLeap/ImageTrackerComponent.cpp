#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeap/ImageTrackerComponent.h"

namespace ue5parse::runtime {

UImageTrackerComponent::UImageTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

}
