#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeap/MeshTrackerComponent.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"

namespace ue5parse::runtime {

UMeshTrackerComponent::UMeshTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BoundingVolume = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("BoundingVolume"));
    BoundingVolume->SetupAttachment(this);
    BoundingVolume->BoxExtent = FVector(500.0f, 500.0f, 500.0f);
}

}
