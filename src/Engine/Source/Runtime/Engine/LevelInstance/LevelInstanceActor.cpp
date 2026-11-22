#include "Engine/Source/Runtime/Engine/LevelInstance/LevelInstanceActor.h"

#include "Engine/Source/Runtime/Engine/LevelInstance/LevelInstanceComponent.h"

namespace ue5parse::runtime {

ALevelInstance::ALevelInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LevelInstanceComponent = ObjectInitializer.CreateDefaultSubobject<ULevelInstanceComponent>(FName("Root"));
    RootComponent = LevelInstanceComponent;
    RootComponent->Mobility = EComponentMobility::Static;
}

void ALevelInstance::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (IsTemplate())
        return;
    Ar << LevelInstanceActorGuid;
}

}
