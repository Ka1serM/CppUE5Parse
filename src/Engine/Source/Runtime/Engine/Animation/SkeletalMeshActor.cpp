#include "Engine/Source/Runtime/Engine/Animation/SkeletalMeshActor.h"

#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

ASkeletalMeshActor::ASkeletalMeshActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SkeletalMeshComponent = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(FName("SkeletalMeshComponent0"));
    SkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
    RootComponent = SkeletalMeshComponent;
}

}
