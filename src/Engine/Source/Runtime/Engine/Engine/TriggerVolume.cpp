#include "Engine/Source/Runtime/Engine/Engine/TriggerVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

ATriggerVolume::ATriggerVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("Trigger");
    bColored = true;
    BrushColor = FColor(100, 255, 100, 255);
}

}
