#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeapPlanes/Source/MagicLeapPlanesComponent.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"

namespace ue5parse::runtime {

UMagicLeapPlanesComponent::UMagicLeapPlanesComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SearchVolume = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("SearchVolume"));
    SearchVolume->SetupAttachment(this);
    SearchVolume->BoxExtent = FVector(1000.0f, 1000.0f, 1000.0f);
}

}
