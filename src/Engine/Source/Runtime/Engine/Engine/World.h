#pragma once

#include <functional>
#include <memory>

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/LatentActionManager.h"
#include "Engine/Source/Runtime/Engine/TickTaskManagerInterface.h"
#include "Engine/Source/Runtime/Engine/TimerManager.h"

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
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UNetDriver;

namespace EWorldType {

enum Type : int32 { None, Game, Editor, PIE, EditorPreview, GamePreview, GameRPC, Inactive };

}

struct FActorSpawnParameters {
    FName Name;
    AActor* Template = nullptr;
    AActor* Owner = nullptr;
    APawn* Instigator = nullptr;
    ULevel* OverrideLevel = nullptr;
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
    ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
    bool bRemoteOwned = false;
    bool bNoFail = false;
    bool bDeferConstruction = false;
    EObjectFlags ObjectFlags = RF_Transactional;
};

UCLASS()
class UWorld : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<ULevel> PersistentLevel = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<ULevelStreaming>> StreamingLevels;
    UPROPERTY()
    TObjectPtr<UNetDriver> NetDriver = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<ULevel>> Levels;
    UPROPERTY()
    TObjectPtr<AGameStateBase> GameState = nullptr;
    UPROPERTY()
    TObjectPtr<AGameModeBase> AuthorityGameMode = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialParameterCollectionInstance>> ParameterCollectionInstances;

    TArray<UObject*> ExtraReferencedObjects;
    EWorldType::Type WorldType = EWorldType::None;
    double TimeSeconds = 0;
    double RealTimeSeconds = 0;
    float DeltaTimeSeconds = 0;
    bool bBegunPlay = false;
    bool bDropDetail = false;

    ~UWorld() override;
    void Serialize(FArchive& Ar) override;
    UWorld* GetWorld() const override { return const_cast<UWorld*>(this); }
    const TArray<TObjectPtr<ULevelStreaming>>& GetStreamingLevels() const { return StreamingLevels; }

    static UWorld* CreateWorld(EWorldType::Type InWorldType, FName WorldName = FName());
    void AddLevel(ULevel* Level);
    void InitializeActor(AActor* Actor);
    void BeginPlay();
    bool HasBegunPlay() const { return bBegunPlay; }
    bool IsGameWorld() const { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview; }

    AActor* SpawnActor(UClass* Class, const FTransform* Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
    template <class T>
    T* SpawnActor(UClass* Class, const FTransform& Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) {
        return Cast<T>(SpawnActor(Class, &Transform, SpawnParameters));
    }
    bool DestroyActor(AActor* Actor, bool bNetForce = false);

    void Tick(ELevelTick TickType, float DeltaSeconds);

    AGameStateBase* GetGameState() const { return GameState; }
    void SetGameState(AGameStateBase* NewGameState) { GameState = NewGameState; }
    AGameModeBase* GetAuthGameMode() const { return AuthorityGameMode; }
    AWorldSettings* GetWorldSettings() const;
    bool IsPaused() const;
    FString GetMapName() const;

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
    TArray<AActor*> GetActors() const;
    UMaterialParameterCollectionInstance* GetParameterCollectionInstance(const UMaterialParameterCollection* Collection);

private:
    FTimerManager TimerManager;
    FLatentActionManager LatentActionManager;
    FTickTaskManager TickTaskManager;
    TArray<AController*> ControllerList;
    TArray<APlayerController*> PlayerControllerList;
    std::vector<std::function<void(AActor*)>> ActorSpawnedHandlers;
};

}
