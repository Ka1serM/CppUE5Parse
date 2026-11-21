#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionRenderLevelSetActor.h"

#include "Engine/Source/Runtime/Engine/Components/PostProcessComponent.h"

namespace ue5parse::runtime {

AGeometryCollectionRenderLevelSetActor::AGeometryCollectionRenderLevelSetActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PostProcessComponent = ObjectInitializer.CreateDefaultSubobject<UPostProcessComponent>(FName("PostProcessComponent0"));
    RootComponent = PostProcessComponent;
}

}
