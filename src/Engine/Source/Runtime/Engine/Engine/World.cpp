#include "Engine/Source/Runtime/Engine/Engine/World.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Engine/Level.h"
#include "Engine/Source/Runtime/Engine/Engine/LevelStreaming.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PlayerController.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/LatentActions.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialParameterCollection.h"

namespace ue5parse::runtime {

UWorld::~UWorld() = default;

void UWorld::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    UObject* Level = nullptr;
    Ar << Level;
    PersistentLevel = Cast<ULevel>(Level);
    Ar << ExtraReferencedObjects;
    const int32 Count = Ar.ReadCount(4);
    StreamingLevels.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        UObject* Streaming = nullptr;
        Ar << Streaming;
        StreamingLevels.Add(Cast<ULevelStreaming>(Streaming));
    }
}

UWorld* UWorld::CreateWorld(EWorldType::Type InWorldType, FName WorldName) {
    if (WorldName.isNone()) WorldName = FName("Untitled");
    UPackage* Package = CreatePackage(FString("/Temp/") + NameToString(WorldName));
    UWorld* World = NewObject<UWorld>(Package, UWorld::StaticClass(), WorldName, RF_Transient);
    World->WorldType = InWorldType;
    World->PersistentLevel = NewObject<ULevel>(World, ULevel::StaticClass(), FName("PersistentLevel"), RF_Transient);
    World->AddLevel(World->PersistentLevel);
    return World;
}

void UWorld::AddLevel(ULevel* Level) {
    if (Level == nullptr || Levels.Contains(Level)) return;
    Level->OwningWorld = this;
    Levels.Add(Level);
}

void UWorld::InitializeActor(AActor* Actor) {
    if (Actor == nullptr || Actor->IsGarbage()) return;
    ULevel* Level = Actor->GetLevel();
    if (Level == nullptr) return;
    AddLevel(Level);
    if (!Actor->IsActorInitialized()) {
        Actor->RegisterAllComponents();
        Actor->InitializeComponents();
        Actor->PostInitializeComponents();
    }
    if (bBegunPlay && !Actor->HasActorBegunPlay()) Actor->DispatchBeginPlay();
}

void UWorld::BeginPlay() {
    if (bBegunPlay) return;
    bBegunPlay = true;
    for (AActor* Actor : GetActors())
        if (Actor != nullptr && Actor->IsActorInitialized()) Actor->DispatchBeginPlay();
}

AWorldSettings* UWorld::GetWorldSettings() const { return PersistentLevel ? Cast<AWorldSettings>(PersistentLevel->WorldSettings) : nullptr; }

bool UWorld::IsPaused() const {
    const AWorldSettings* Settings = GetWorldSettings();
    return Settings != nullptr && Settings->GetPauserPlayerState() != nullptr;
}

FString UWorld::GetMapName() const {
    const FString PackageName = GetOutermost()->GetName();
    const int32 Slash = PackageName.Find(FString("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    return Slash == INDEX_NONE ? PackageName : PackageName.Mid(Slash + 1);
}

void UWorld::AddController(AController* Controller) {
    if (Controller == nullptr || ControllerList.Contains(Controller)) return;
    ControllerList.Add(Controller);
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) PlayerControllerList.Add(PlayerController);
}

void UWorld::RemoveController(AController* Controller) {
    ControllerList.Remove(Controller);
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) PlayerControllerList.Remove(PlayerController);
}

ENetMode UWorld::GetNetMode() const { return NetDriver ? NM_Client : NM_Standalone; }

TArray<AActor*> UWorld::GetActors() const {
    TArray<AActor*> Actors;
    for (ULevel* Level : Levels)
        if (Level != nullptr)
            for (AActor* Actor : Level->Actors)
                if (Actor != nullptr && !Actor->IsGarbage()) Actors.Add(Actor);
    return Actors;
}

UMaterialParameterCollectionInstance* UWorld::GetParameterCollectionInstance(const UMaterialParameterCollection* Collection) {
    if (Collection == nullptr)
        return nullptr;
    for (UMaterialParameterCollectionInstance* Instance : ParameterCollectionInstances)
        if (Instance->Collection == Collection)
            return Instance;
    auto* Instance = NewObject<UMaterialParameterCollectionInstance>(this);
    Instance->Collection = const_cast<UMaterialParameterCollection*>(Collection);
    ParameterCollectionInstances.Add(Instance);
    return Instance;
}

AActor* UWorld::SpawnActor(UClass* Class, const FTransform* Transform, const FActorSpawnParameters& SpawnParameters) {
    if (Class == nullptr || !Class->IsChildOf(AActor::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
    ULevel* LevelToSpawnIn = SpawnParameters.OverrideLevel ? SpawnParameters.OverrideLevel : PersistentLevel.Get();
    if (LevelToSpawnIn == nullptr) return nullptr;
    AActor* Template = SpawnParameters.Template ? SpawnParameters.Template : Cast<AActor>(Class->GetDefaultObject());
    const FTransform UserTransform = Transform ? *Transform : FTransform::Identity;
    AActor* Actor = NewObject<AActor>(LevelToSpawnIn, Class, SpawnParameters.Name, SpawnParameters.ObjectFlags, Template);
    if (Actor == nullptr) return nullptr;
    LevelToSpawnIn->Actors.Add(Actor);
    Actor->PostSpawnInitialize(UserTransform, SpawnParameters.Owner, SpawnParameters.Instigator, SpawnParameters.bRemoteOwned,
                               SpawnParameters.bNoFail, SpawnParameters.bDeferConstruction, SpawnParameters.TransformScaleMethod);
    if (Actor->IsGarbage() && !SpawnParameters.bNoFail) return nullptr;
    for (const auto& [Handle, Handler] : ActorSpawnedHandlers) Handler(Actor);
    return Actor;
}

uint64 UWorld::AddOnActorSpawnedHandler(std::function<void(AActor*)> Handler) {
    const uint64 Handle = ++NextActorSpawnedHandle;
    ActorSpawnedHandlers.emplace_back(Handle, std::move(Handler));
    return Handle;
}

void UWorld::RemoveOnActorSpawnedHandler(const uint64 Handle) {
    std::erase_if(ActorSpawnedHandlers, [&](const auto& Entry) { return Entry.first == Handle; });
}

bool UWorld::DestroyActor(AActor* Actor, bool bNetForce) {
    if (Actor == nullptr || Actor->IsActorBeingDestroyed()) return true;
    if (Actor->GetLocalRole() != ROLE_Authority && !bNetForce && !Actor->bTearOff && !Actor->IsSpawnedActor()) return false;
    Actor->bActorIsBeingDestroyed = true;
    Actor->Destroyed();
    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors, true, false);
    for (AActor* Attached : AttachedActors) Attached->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    if (Actor->GetOwner() != nullptr) Actor->SetOwner(nullptr);
    Actor->RegisterAllActorTickFunctions(false, true);
    Actor->UnregisterAllComponents();
    if (ULevel* Level = Actor->GetLevel()) {
        const int32 Index = Level->Actors.Find(Actor);
        if (Index != INDEX_NONE) Level->Actors[Index] = nullptr;
    }
    for (UActorComponent* Component : TArray<UActorComponent*>(Actor->GetComponents()))
        if (Component != nullptr) Component->MarkAsGarbage();
    Actor->MarkAsGarbage();
    return true;
}

void UWorld::Tick(ELevelTick TickType, float DeltaSeconds) {
    RealTimeSeconds += DeltaSeconds;
    if (const AWorldSettings* Settings = GetWorldSettings()) DeltaSeconds *= Settings->GetEffectiveTimeDilation();
    DeltaTimeSeconds = DeltaSeconds;
    if (!IsPaused()) TimeSeconds += DeltaSeconds;
    LatentActionManager.BeginFrame();
    TickTaskManager.StartFrame(this, DeltaSeconds, TickType);
    TickTaskManager.RunTickGroup(TG_PrePhysics);
    TickTaskManager.RunTickGroup(TG_StartPhysics);
    TickTaskManager.RunTickGroup(TG_DuringPhysics);
    TickTaskManager.RunTickGroup(TG_EndPhysics);
    TickTaskManager.RunTickGroup(TG_PostPhysics);
    TimerManager.Tick(DeltaSeconds);
    TickTaskManager.RunTickGroup(TG_PostUpdateWork);
    TickTaskManager.RunTickGroup(TG_LastDemotable);
    TickTaskManager.EndFrame();
    LatentActionManager.ProcessLatentActions(nullptr, DeltaSeconds);
}

}
