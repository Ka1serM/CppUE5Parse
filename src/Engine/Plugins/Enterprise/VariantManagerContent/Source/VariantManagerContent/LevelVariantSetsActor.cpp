#include "Engine/Plugins/Enterprise/VariantManagerContent/Source/VariantManagerContent/LevelVariantSetsActor.h"

namespace ue5parse::runtime {

ALevelVariantSetsActor::ALevelVariantSetsActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
