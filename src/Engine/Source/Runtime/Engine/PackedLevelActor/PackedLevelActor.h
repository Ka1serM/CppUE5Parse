#pragma once

#include "Engine/Source/Runtime/Engine/LevelInstance/LevelInstanceActor.h"

#include "PackedLevelActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APackedLevelActor : public ALevelInstance {
    GENERATED_BODY()
public:
    explicit APackedLevelActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
