#include "Engine/Plugins/VirtualProduction/GeometryMask/Source/GeometryMask/GeometryMaskCanvasActor.h"

namespace ue5parse::runtime {

AGeometryMaskCanvasActor::AGeometryMaskCanvasActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("Root"));
}

}
