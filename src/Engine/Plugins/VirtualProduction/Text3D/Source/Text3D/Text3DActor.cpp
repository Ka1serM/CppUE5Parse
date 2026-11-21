#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Text3DActor.h"

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Text3DComponent.h"

namespace ue5parse::runtime {

AText3DActor::AText3DActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Text3DComponent = ObjectInitializer.CreateDefaultSubobject<UText3DComponent>(FName("Text3DComponent"));
    RootComponent = Text3DComponent;
}

}
