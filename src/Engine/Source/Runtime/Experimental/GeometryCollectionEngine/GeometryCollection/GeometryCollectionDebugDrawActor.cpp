#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionDebugDrawActor.h"

namespace ue5parse::runtime {

AGeometryCollectionDebugDrawActor::AGeometryCollectionDebugDrawActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComponent"));
}

}
