#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionBoneActor.h"

#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SphereComponent.h"
#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

ALeapMotionBoneActor::ALeapMotionBoneActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(FName("RootSphereComponent"));
    PrimitiveComponent = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(FName("CapsuleComponent"));
    PrimitiveComponent->SetupAttachment(RootComponent);
    StaticMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent"));
    StaticMeshComponent->SetupAttachment(RootComponent);
}

}
