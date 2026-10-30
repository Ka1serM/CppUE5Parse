#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Level.generated.h"

namespace ue5parse::runtime {

class UWorld;

// ULevel: the actors of one level of a world (UE's Engine/Level.h). The actor list, URL and BSP model are serialized
// natively.
UCLASS()
class ULevel : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<AActor> WorldSettings = nullptr;
    // The world the level is part of: its own world, or the world it was added to (UE's OwningWorld).
    UPROPERTY()
    TObjectPtr<UWorld> OwningWorld = nullptr;

    // The actors, in the order of the level; a destroyed actor leaves a null entry.
    TArray<AActor*> Actors;
    FURL URL;
    // The BSP model and its components, and the level Blueprint's actor. The runtime has no classes for them.
    UObject* Model = nullptr;
    TArray<UObject*> ModelComponents;
    UObject* LevelScriptActor = nullptr;

    void Serialize(FArchive& Ar) override;
    // A loaded level is part of the world it is in, until a world adds it.
    void PostLoad() override;
};

}
