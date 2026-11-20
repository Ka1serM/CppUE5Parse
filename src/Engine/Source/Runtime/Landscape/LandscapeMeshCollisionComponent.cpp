#include "Engine/Source/Runtime/Landscape/LandscapeMeshCollisionComponent.h"

namespace ue5parse::runtime {

ULandscapeMeshCollisionComponent::ULandscapeMeshCollisionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAlwaysCreatePhysicsState = true;
}

void ULandscapeMeshCollisionComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() < EUnrealEngineObjectUE4Version::ADD_COOKED_TO_LANDSCAPE)
        return;
    bool bCooked = false;
    Ar << bCooked;
}

}
