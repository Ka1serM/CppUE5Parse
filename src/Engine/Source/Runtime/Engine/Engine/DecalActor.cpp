#include "Engine/Source/Runtime/Engine/Engine/DecalActor.h"

#include "Engine/Source/Runtime/Engine/Components/DecalComponent.h"

namespace ue5parse::runtime {

ADecalActor::ADecalActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Decal = ObjectInitializer.CreateDefaultSubobject<UDecalComponent>(FName("NewDecalComponent"));
    Decal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    RootComponent = Decal;
}

}
