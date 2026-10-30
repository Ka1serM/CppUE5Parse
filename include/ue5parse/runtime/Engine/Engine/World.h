#pragma once

#include <functional>
#include <memory>

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/Engine/LatentActionManager.h"
#include "ue5parse/runtime/Engine/TickTaskManagerInterface.h"
#include "ue5parse/runtime/Engine/TimerManager.h"

#include "World.generated.h"

namespace ue5parse::runtime {

class AActor;
class AController;
class AGameModeBase;
class AGameStateBase;
class APawn;
class APlayerController;
class AWorldSettings;
class ULevel;
class ULevelStreaming;
class UNetDriver;

namespace EWorldType {
enum Type : int32 { None, Game, Editor, PIE, EditorPreview, GamePreview, GameRPC, Inactive };
}

// FActorSpawnParameters: how UWorld::SpawnActor makes an actor (UE's Engine/World.h).
struct FActorSpawnParameters {
    FName Name;
    AActor* Template = nullptr;
    AActor* Owner = nullptr;
    APawn* Instigator = nullptr;
    ULevel* OverrideLevel = nullptr;
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
    ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
    // The actor is a client's copy of a server actor (UPackageMapClient::SerializeNewActor): its roles swap.
    bool bRemoteOwned = false;
    bool bNoFail = false;
    // The caller finishes the spawn with AActor::FinishSpawning (UE's bDeferConstruction).
    bool bDeferConstruction = false;
    EObjectFlags ObjectFlags = RF_Transactional;
};

// UWorld: a map: its persistent level and the levels it streams in (UE's Engine/World.h). A game world also runs
// actors: it spawns them, ticks them by tick group, and runs its timers and latent actions.
UCLASS()
class UWorld : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    ULevel* PersistentLevel = nullptr;
    UPROPERTY()
    TArray<ULevelStreaming*> StreamingLevels;
    UPROPERTY()
    UNetDriver* NetDriver = nullptr;
    UPROPERTY()
    TArray<ULevel*> Levels;
    UPROPERTY()
    AGameStateBase* GameState = nullptr;
    // The game mode exists on the server only (UE's AuthorityGameMode).
    UPROPERTY()
    AGameModeBase* AuthorityGameMode = nullptr;

    TArray<UObject*> ExtraReferencedObjects;
    EWorldType::Type WorldType = EWorldType::None;
    // The game time: the sum of the ticks' delta times (UE's TimeSeconds), and the last tick's.
    double TimeSeconds = 0;
    double RealTimeSeconds = 0;
    float DeltaTimeSeconds = 0;
    bool bBegunPlay = false;

    ~UWorld() override;
    void Serialize(FArchive& Ar) override;
    UWorld* GetWorld() const override { return const_cast<UWorld*>(this); }
    const TArray<ULevelStreaming*>& GetStreamingLevels() const { return StreamingLevels; }

    // A new game world with an empty persistent level in its own transient package (UE's UWorld::CreateWorld).
    static UWorld* CreateWorld(EWorldType::Type InWorldType, FName WorldName = FName());
    // Adds a level of another package to the world: its OwningWorld becomes this (UE's AddToWorld, without making its
    // actors play: a level actor joins when something uses it, see InitializeActor).
    void AddLevel(ULevel* Level);
    // Registers a placed actor's components and begins its play when the world plays (UE's level actor path of
    // AddToWorld and InitializeActorsForPlay, for one actor).
    void InitializeActor(AActor* Actor);
    // UWorld::BeginPlay: the actors that exist begin play.
    void BeginPlay();
    bool HasBegunPlay() const { return bBegunPlay; }
    bool IsGameWorld() const { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview; }

    AActor* SpawnActor(UClass* Class, const FTransform* Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
    template <class T>
    T* SpawnActor(UClass* Class, const FTransform& Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) {
        return Cast<T>(SpawnActor(Class, &Transform, SpawnParameters));
    }
    // AActor::Destroy: EndPlay, components unregistered, the actor removed from its level and marked as garbage.
    bool DestroyActor(AActor* Actor, bool bNetForce = false);
    // The callbacks UWorld::AddOnActorSpawnedHandler adds; they run after each SpawnActor.
    void AddOnActorSpawnedHandler(std::function<void(AActor*)> Handler) { ActorSpawnedHandlers.push_back(std::move(Handler)); }

    // UWorld::Tick: the tick groups, the timers after TG_PostPhysics, the latent actions of the objects that did not
    // tick, then the end of the frame.
    void Tick(ELevelTick TickType, float DeltaSeconds);

    AGameStateBase* GetGameState() const { return GameState; }
    void SetGameState(AGameStateBase* NewGameState) { GameState = NewGameState; }
    AGameModeBase* GetAuthGameMode() const { return AuthorityGameMode; }
    // The settings actor of the persistent level.
    AWorldSettings* GetWorldSettings() const;
    // Paused by a player (UE's IsPaused: the world settings name who paused).
    bool IsPaused() const;
    // The short name of the world's package ("Ascent" of /Game/Maps/Ascent/Ascent).
    FString GetMapName() const;

    // The controllers join when their components are initialized and leave when they are destroyed (UE's
    // AddController and RemoveController).
    void AddController(AController* Controller);
    void RemoveController(AController* Controller);
    const TArray<AController*>& GetControllers() const { return ControllerList; }
    const TArray<APlayerController*>& GetPlayerControllers() const { return PlayerControllerList; }
    APlayerController* GetFirstPlayerController() const { return PlayerControllerList.Num() > 0 ? PlayerControllerList[0] : nullptr; }

    double GetTimeSeconds() const { return TimeSeconds; }
    double GetRealTimeSeconds() const { return RealTimeSeconds; }
    float GetDeltaSeconds() const { return DeltaTimeSeconds; }
    ENetMode GetNetMode() const;
    FTimerManager& GetTimerManager() { return TimerManager; }
    FLatentActionManager& GetLatentActionManager() { return LatentActionManager; }
    FTickTaskManager& GetTickTaskManager() { return TickTaskManager; }
    // The actors of every level of the world, in level order (TActorIterator).
    TArray<AActor*> GetActors() const;

private:
    FTimerManager TimerManager;
    FLatentActionManager LatentActionManager;
    FTickTaskManager TickTaskManager;
    TArray<AController*> ControllerList;
    TArray<APlayerController*> PlayerControllerList;
    std::vector<std::function<void(AActor*)>> ActorSpawnedHandlers;
};

}
