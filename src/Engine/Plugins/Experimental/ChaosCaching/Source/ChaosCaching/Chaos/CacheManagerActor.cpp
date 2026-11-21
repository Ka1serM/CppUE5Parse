#include "Engine/Plugins/Experimental/ChaosCaching/Source/ChaosCaching/Chaos/CacheManagerActor.h"

namespace ue5parse::runtime {

AChaosCacheManager::AChaosCacheManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("Root"));
    RootComponent->Mobility = EComponentMobility::Static;
}

}
