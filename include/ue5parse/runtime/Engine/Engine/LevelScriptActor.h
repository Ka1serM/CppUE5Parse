#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "LevelScriptActor.generated.h"

namespace ue5parse::runtime {

// ALevelScriptActor: the actor of a level's Blueprint (UE's Engine/LevelScriptActor.h). Replication does not destroy it.
UCLASS()
class ALevelScriptActor : public AActor {
    GENERATED_BODY()
};

}
