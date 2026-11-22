#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeapImageTracker/MagicLeapImageTrackerComponent.h"

namespace ue5parse::runtime {

UMagicLeapImageTrackerComponent::UMagicLeapImageTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

}
