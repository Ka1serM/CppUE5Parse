#include "Engine/Source/Runtime/Engine/Engine/BlockingVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

ABlockingVolume::ABlockingVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->bCanEverAffectNavigation = true;
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("InvisibleWall");
}

}
