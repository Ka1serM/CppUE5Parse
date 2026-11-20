#include "Engine/Plugins/Runtime/GeometryCache/Source/GeometryCache/GeometryCacheActor.h"

#include "Engine/Plugins/Runtime/GeometryCache/Source/GeometryCache/GeometryCacheComponent.h"

namespace ue5parse::runtime {

AGeometryCacheActor::AGeometryCacheActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GeometryCacheComponent = ObjectInitializer.CreateDefaultSubobject<UGeometryCacheComponent>(FName("GeometryCacheComponent"));
    RootComponent = GeometryCacheComponent;
}

}
