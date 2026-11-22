#include "Engine/Source/Runtime/Landscape/LandscapeProxy.h"

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Landscape/Landscape.h"
#include "Engine/Source/Runtime/Landscape/LandscapeStreamingProxy.h"

namespace ue5parse::runtime {

ALandscapeProxy::ALandscapeProxy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bReplicates = false;
    BodyInstance.CollisionProfileName = FName("BlockAll");
    //! UE 4.23 and older assign the scale directly, so ComponentToWorld does not change.
    RootComponent->SetRelativeScale3D_Direct(FVector(128.0, 128.0, 256.0));
}

ALandscape::ALandscape(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bIsProxy = false;
}

ALandscapeStreamingProxy::ALandscapeStreamingProxy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

}
