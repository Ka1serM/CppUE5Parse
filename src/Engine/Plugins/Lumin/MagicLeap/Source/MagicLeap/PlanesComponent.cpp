#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeap/PlanesComponent.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"

namespace ue5parse::runtime {

UPlanesComponent::UPlanesComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SearchVolume = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("SearchVolume"));
    SearchVolume->SetupAttachment(this);
    SearchVolume->BoxExtent = FVector(1000.0f, 1000.0f, 1000.0f);
}

}
