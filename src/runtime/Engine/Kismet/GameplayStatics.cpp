#include "ue5parse/runtime/Engine/Kismet/GameplayStatics.h"

#include <limits>

#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Engine/LevelStreaming.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Character.h"
#include "ue5parse/runtime/Engine/GameFramework/GameModeBase.h"
#include "ue5parse/runtime/Engine/GameFramework/GameStateBase.h"
#include "ue5parse/runtime/Engine/GameFramework/PlayerController.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

namespace {

UWorld* GetWorldFromContextObject(const UObject* WorldContextObject) {
    return WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
}

}

// --- Spawning ---

UObject* UGameplayStatics::SpawnObject(TSubclassOf<UObject> ObjectClass, UObject* Outer) {
    UClass* Class = ObjectClass.Get();
    if (Class == nullptr || Outer == nullptr) return nullptr;
    if (Class->ClassWithin != nullptr && !Outer->IsA(Class->ClassWithin)) return nullptr;
    return NewObject<UObject>(Outer, Class, FName(), RF_NoFlags);
}

// The instigator is the context pawn, or the context actor's instigator.
AActor* UGameplayStatics::BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass,
                                                           const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride,
                                                           AActor* Owner, ESpawnActorScaleMethod TransformScaleMethod) {
    UClass* Class = ActorClass.Get();
    if (Class == nullptr) return nullptr;
    UObject* MutableWorldContextObject = const_cast<UObject*>(WorldContextObject);
    APawn* AutoInstigator = Cast<APawn>(MutableWorldContextObject);
    if (AutoInstigator == nullptr)
        if (AActor* ContextActor = Cast<AActor>(MutableWorldContextObject)) AutoInstigator = ContextActor->GetInstigator();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr) return nullptr;
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = Owner;
    SpawnParameters.Instigator = AutoInstigator;
    SpawnParameters.SpawnCollisionHandlingOverride = CollisionHandlingOverride;
    SpawnParameters.TransformScaleMethod = TransformScaleMethod;
    SpawnParameters.bDeferConstruction = true;
    return World->SpawnActor(Class, &SpawnTransform, SpawnParameters);
}

AActor* UGameplayStatics::FinishSpawningActor(AActor* Actor, const FTransform& SpawnTransform, ESpawnActorScaleMethod) {
    if (Actor != nullptr) Actor->FinishSpawning(SpawnTransform, false);
    return Actor;
}

// --- Finding actors ---

AActor* UGameplayStatics::GetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr) return nullptr;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get())) return Actor;
    return nullptr;
}

void UGameplayStatics::GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr) return;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get())) OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsWithInterface(const UObject* WorldContextObject, TSubclassOf<UInterface> Interface, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Interface.Get() == nullptr || World == nullptr) return;
    for (AActor* Actor : World->GetActors())
        if (Actor->GetClass()->ImplementsInterface(Interface.Get())) OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsWithTag(const UObject* WorldContextObject, FName Tag, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Tag.isNone() || World == nullptr) return;
    for (AActor* Actor : World->GetActors())
        if (Actor->ActorHasTag(Tag)) OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsOfClassWithTag(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FName Tag,
                                                  TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr) return;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get()) && Actor->ActorHasTag(Tag)) OutActors.Add(Actor);
}

AActor* UGameplayStatics::FindNearestActor(FVector Origin, const TArray<AActor*>& ActorsToCheck, float& Distance) {
    AActor* NearestActor = nullptr;
    double NearestDistanceSquared = std::numeric_limits<float>::max();
    Distance = std::numeric_limits<float>::max();
    for (AActor* Actor : ActorsToCheck) {
        if (Actor == nullptr) continue;
        const double DistanceSquared = (Origin - Actor->GetActorLocation()).SizeSquared();
        if (DistanceSquared >= NearestDistanceSquared) continue;
        NearestActor = Actor;
        NearestDistanceSquared = DistanceSquared;
    }
    if (NearestActor != nullptr) Distance = static_cast<float>(std::sqrt(NearestDistanceSquared));
    return NearestActor;
}

// --- Players ---

APlayerController* UGameplayStatics::GetPlayerController(const UObject* WorldContextObject, int32 PlayerIndex) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr || PlayerIndex < 0 || PlayerIndex >= World->GetPlayerControllers().Num()) return nullptr;
    return World->GetPlayerControllers()[PlayerIndex];
}

APawn* UGameplayStatics::GetPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex) {
    APlayerController* PlayerController = GetPlayerController(WorldContextObject, PlayerIndex);
    return PlayerController != nullptr ? PlayerController->GetPawnOrSpectator() : nullptr;
}

ACharacter* UGameplayStatics::GetPlayerCharacter(const UObject* WorldContextObject, int32 PlayerIndex) {
    APlayerController* PlayerController = GetPlayerController(WorldContextObject, PlayerIndex);
    return PlayerController != nullptr ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
}

// --- Levels and game ---

// A short name ("Ascent_Art") matches the end of a streaming level's package path; a long one the whole path.
ULevelStreaming* UGameplayStatics::GetStreamingLevel(const UObject* WorldContextObject, FName PackageName) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (PackageName.isNone() || World == nullptr) return nullptr;
    const FString SearchPackageName = NameToString(PackageName);
    const bool bShortName = !SearchPackageName.StartsWith(FString("/"));
    const FString PackageNameWithSlash = FString("/") + SearchPackageName;
    for (ULevelStreaming* LevelStreaming : World->GetStreamingLevels()) {
        if (LevelStreaming == nullptr) continue;
        const FName LevelPackageName = LevelStreaming->GetWorldAssetPackageFName();
        if (bShortName ? NameToString(LevelPackageName).EndsWith(PackageNameWithSlash, ESearchCase::IgnoreCase) : LevelPackageName == PackageName)
            return LevelStreaming;
    }
    return nullptr;
}

// A cooked world has no streaming prefix to remove.
FString UGameplayStatics::GetCurrentLevelName(const UObject* WorldContextObject, bool) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetMapName() : FString();
}

AGameModeBase* UGameplayStatics::GetGameMode(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetAuthGameMode() : nullptr;
}

AGameStateBase* UGameplayStatics::GetGameState(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetGameState() : nullptr;
}

UClass* UGameplayStatics::GetObjectClass(const UObject* Object) { return Object != nullptr ? Object->GetClass() : nullptr; }

bool UGameplayStatics::ObjectIsA(const UObject* Object, TSubclassOf<UObject> ObjectClass) {
    return Object != nullptr && ObjectClass.Get() != nullptr && Object->IsA(ObjectClass.Get());
}

float UGameplayStatics::GetGlobalTimeDilation(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    const AWorldSettings* Settings = World != nullptr ? World->GetWorldSettings() : nullptr;
    return Settings != nullptr ? Settings->TimeDilation : 1.f;
}

void UGameplayStatics::SetGlobalTimeDilation(const UObject* WorldContextObject, float TimeDilation) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (AWorldSettings* Settings = World != nullptr ? World->GetWorldSettings() : nullptr) Settings->SetTimeDilation(TimeDilation);
}

bool UGameplayStatics::IsGamePaused(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr && World->IsPaused();
}

// --- Hit results ---

void UGameplayStatics::BreakHitResult(const FHitResult& Hit, bool& bBlockingHit, bool& bInitialOverlap, float& Time, float& Distance,
                                      FVector& Location, FVector& ImpactPoint, FVector& Normal, FVector& ImpactNormal, UPhysicalMaterial*& PhysMat,
                                      AActor*& HitActor, UPrimitiveComponent*& HitComponent, FName& HitBoneName, FName& BoneName, int32& HitItem,
                                      int32& ElementIndex, int32& FaceIndex, FVector& TraceStart, FVector& TraceEnd) {
    bBlockingHit = Hit.bBlockingHit;
    bInitialOverlap = Hit.bStartPenetrating;
    Time = Hit.Time;
    Distance = Hit.Distance;
    Location = Hit.Location;
    ImpactPoint = Hit.ImpactPoint;
    Normal = Hit.Normal;
    ImpactNormal = Hit.ImpactNormal;
    PhysMat = Hit.PhysMaterial.Get();
    HitActor = Hit.GetActor();
    HitComponent = Hit.GetComponent();
    HitBoneName = Hit.BoneName;
    BoneName = Hit.MyBoneName;
    HitItem = Hit.Item;
    ElementIndex = Hit.ElementIndex;
    TraceStart = Hit.TraceStart;
    TraceEnd = Hit.TraceEnd;
    FaceIndex = Hit.FaceIndex;
}

FHitResult UGameplayStatics::MakeHitResult(bool bBlockingHit, bool bInitialOverlap, float Time, float Distance, FVector Location, FVector ImpactPoint,
                                           FVector Normal, FVector ImpactNormal, UPhysicalMaterial* PhysMat, AActor* HitActor,
                                           UPrimitiveComponent* HitComponent, FName HitBoneName, FName BoneName, int32 HitItem, int32 ElementIndex,
                                           int32 FaceIndex, FVector TraceStart, FVector TraceEnd) {
    FHitResult Hit;
    Hit.bBlockingHit = bBlockingHit;
    Hit.bStartPenetrating = bInitialOverlap;
    Hit.Time = Time;
    Hit.Distance = Distance;
    Hit.Location = Location;
    Hit.ImpactPoint = ImpactPoint;
    Hit.Normal = Normal;
    Hit.ImpactNormal = ImpactNormal;
    Hit.PhysMaterial = PhysMat;
    Hit.HitObjectHandle.Actor = HitActor;
    Hit.Component = HitComponent;
    Hit.BoneName = HitBoneName;
    Hit.MyBoneName = BoneName;
    Hit.Item = HitItem;
    Hit.ElementIndex = static_cast<uint8>(ElementIndex);
    Hit.FaceIndex = FaceIndex;
    Hit.TraceStart = TraceStart;
    Hit.TraceEnd = TraceEnd;
    return Hit;
}

// --- Time and platform ---

double UGameplayStatics::GetWorldDeltaSeconds(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetDeltaSeconds() : 0.0;
}

double UGameplayStatics::GetTimeSeconds(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetTimeSeconds() : 0.0;
}

double UGameplayStatics::GetRealTimeSeconds(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr ? World->GetRealTimeSeconds() : 0.0;
}

// The cooked game is the Windows client.
FString UGameplayStatics::GetPlatformName() { return FString("Windows"); }

// The runtime has no command line.
bool UGameplayStatics::HasLaunchOption(const FString&) { return false; }

}
