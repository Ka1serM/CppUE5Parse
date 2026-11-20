#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GameplayCueNotify_Actor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AGameplayCueNotify_Actor : public AActor {
    GENERATED_BODY()
public:
    explicit AGameplayCueNotify_Actor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
