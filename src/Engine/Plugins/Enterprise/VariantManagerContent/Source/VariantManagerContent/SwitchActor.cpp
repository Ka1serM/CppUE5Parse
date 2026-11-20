#include "Engine/Plugins/Enterprise/VariantManagerContent/Source/VariantManagerContent/SwitchActor.h"

namespace ue5parse::runtime {

ASwitchActor::ASwitchActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("Root"));
    RootComponent = SceneComponent;
}

}
