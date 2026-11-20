#include "Engine/Source/Runtime/Engine/Engine/LODActor.h"

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

ALODActor::ALODActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    StaticMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent0"));
    RootComponent = StaticMeshComponent;
}

}
