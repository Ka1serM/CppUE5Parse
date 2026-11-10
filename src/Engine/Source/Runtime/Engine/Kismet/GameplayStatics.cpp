#include "Engine/Source/Runtime/Engine/Kismet/GameplayStatics.h"

#include <limits>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Engine/LevelStreaming.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"
#include "Engine/Source/Runtime/Engine/GameFramework/GameModeBase.h"
#include "Engine/Source/Runtime/Engine/GameFramework/GameStateBase.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PlayerController.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

namespace ue5parse::runtime {

namespace {

UWorld* GetWorldFromContextObject(const UObject* WorldContextObject) {
    return WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
}

}

UObject* UGameplayStatics::SpawnObject(TSubclassOf<UObject> ObjectClass, UObject* Outer) {
    UClass* Class = ObjectClass.Get();
    if (Class == nullptr || Outer == nullptr)
        return nullptr;
    if (Class->ClassWithin != nullptr && !Outer->IsA(Class->ClassWithin))
        return nullptr;
    return NewObject<UObject>(Outer, Class, FName(), RF_NoFlags);
}

AActor* UGameplayStatics::BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform,
                                                           ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner,
                                                           ESpawnActorScaleMethod TransformScaleMethod) {
    UClass* Class = ActorClass.Get();
    if (Class == nullptr)
        return nullptr;
    UObject* MutableWorldContextObject = const_cast<UObject*>(WorldContextObject);
    APawn* AutoInstigator = Cast<APawn>(MutableWorldContextObject);
    if (AutoInstigator == nullptr)
        if (AActor* ContextActor = Cast<AActor>(MutableWorldContextObject))
            AutoInstigator = ContextActor->GetInstigator();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr)
        return nullptr;
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = Owner;
    SpawnParameters.Instigator = AutoInstigator;
    SpawnParameters.SpawnCollisionHandlingOverride = CollisionHandlingOverride;
    SpawnParameters.TransformScaleMethod = TransformScaleMethod;
    SpawnParameters.bDeferConstruction = true;
    return World->SpawnActor(Class, &SpawnTransform, SpawnParameters);
}

AActor* UGameplayStatics::FinishSpawningActor(AActor* Actor, const FTransform& SpawnTransform, ESpawnActorScaleMethod) {
    if (Actor != nullptr)
        Actor->FinishSpawning(SpawnTransform, false);
    return Actor;
}

AActor* UGameplayStatics::GetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr)
        return nullptr;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get()))
            return Actor;
    return nullptr;
}

void UGameplayStatics::GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr)
        return;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get()))
            OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsWithInterface(const UObject* WorldContextObject, TSubclassOf<UInterface> Interface, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Interface.Get() == nullptr || World == nullptr)
        return;
    for (AActor* Actor : World->GetActors())
        if (Actor->GetClass()->ImplementsInterface(Interface.Get()))
            OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsWithTag(const UObject* WorldContextObject, FName Tag, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (Tag.isNone() || World == nullptr)
        return;
    for (AActor* Actor : World->GetActors())
        if (Actor->ActorHasTag(Tag))
            OutActors.Add(Actor);
}

void UGameplayStatics::GetAllActorsOfClassWithTag(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FName Tag, TArray<AActor*>& OutActors) {
    OutActors.Reset();
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (ActorClass.Get() == nullptr || World == nullptr)
        return;
    for (AActor* Actor : World->GetActors())
        if (Actor->IsA(ActorClass.Get()) && Actor->ActorHasTag(Tag))
            OutActors.Add(Actor);
}

AActor* UGameplayStatics::FindNearestActor(FVector Origin, const TArray<AActor*>& ActorsToCheck, float& Distance) {
    AActor* NearestActor = nullptr;
    double NearestDistanceSquared = std::numeric_limits<float>::max();
    Distance = std::numeric_limits<float>::max();
    for (AActor* Actor : ActorsToCheck) {
        if (Actor == nullptr)
            continue;
        const double DistanceSquared = (Origin - Actor->GetActorLocation()).SizeSquared();
        if (DistanceSquared >= NearestDistanceSquared)
            continue;
        NearestActor = Actor;
        NearestDistanceSquared = DistanceSquared;
    }
    if (NearestActor != nullptr)
        Distance = static_cast<float>(std::sqrt(NearestDistanceSquared));
    return NearestActor;
}

APlayerController* UGameplayStatics::GetPlayerController(const UObject* WorldContextObject, int32 PlayerIndex) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (World == nullptr || PlayerIndex < 0 || PlayerIndex >= World->GetPlayerControllers().Num())
        return nullptr;
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

ULevelStreaming* UGameplayStatics::GetStreamingLevel(const UObject* WorldContextObject, FName PackageName) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    if (PackageName.isNone() || World == nullptr)
        return nullptr;
    const FString SearchPackageName = NameToString(PackageName);
    const bool bShortName = !SearchPackageName.StartsWith(FString("/"));
    const FString PackageNameWithSlash = FString("/") + SearchPackageName;
    for (ULevelStreaming* LevelStreaming : World->GetStreamingLevels()) {
        if (LevelStreaming == nullptr)
            continue;
        const FName LevelPackageName = LevelStreaming->GetWorldAssetPackageFName();
        if (bShortName ? NameToString(LevelPackageName).EndsWith(PackageNameWithSlash, ESearchCase::IgnoreCase) : LevelPackageName == PackageName)
            return LevelStreaming;
    }
    return nullptr;
}

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

UClass* UGameplayStatics::GetObjectClass(const UObject* Object) {
    return Object != nullptr ? Object->GetClass() : nullptr;
}

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
    if (AWorldSettings* Settings = World != nullptr ? World->GetWorldSettings() : nullptr)
        Settings->SetTimeDilation(TimeDilation);
}

bool UGameplayStatics::IsGamePaused(const UObject* WorldContextObject) {
    UWorld* World = GetWorldFromContextObject(WorldContextObject);
    return World != nullptr && World->IsPaused();
}

void UGameplayStatics::BreakHitResult(const FHitResult& Hit, bool& bBlockingHit, bool& bInitialOverlap, float& Time, float& Distance, FVector& Location,
                                      FVector& ImpactPoint, FVector& Normal, FVector& ImpactNormal, UPhysicalMaterial*& PhysMat, AActor*& HitActor,
                                      UPrimitiveComponent*& HitComponent, FName& HitBoneName, FName& BoneName, int32& HitItem, int32& ElementIndex,
                                      int32& FaceIndex, FVector& TraceStart, FVector& TraceEnd) {
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

FString UGameplayStatics::GetPlatformName() {
    return FString("Windows");
}

bool UGameplayStatics::HasLaunchOption(const FString&) {
    return false;
}

namespace {

UParticleSystemComponent* CreateParticleSystem(UParticleSystem* EmitterTemplate, UWorld* World, AActor* Actor, bool bAutoDestroy, EPSCPoolMethod PoolingMethod,
                                               bool bIsFirstPerson, uint8 ParticleVariantColor) {
    if (World == nullptr || World->GetNetMode() == NM_DedicatedServer)
        return nullptr;
    UParticleSystemComponent* PSC = nullptr;
    if (PoolingMethod != EPSCPoolMethod::None) {
        PSC = NewObject<UParticleSystemComponent>(World);
        PSC->bAutoDestroy = true;
    } else {
        PSC = NewObject<UParticleSystemComponent>(Actor ? (UObject*)Actor : (UObject*)World);
        PSC->bAutoDestroy = bAutoDestroy;
    }
    PSC->SecondsBeforeInactive = 0.0f;
    PSC->bAutoActivate = false;
    PSC->SetTemplate(EmitterTemplate);
    PSC->bOverrideLODMethod = false;
    PSC->ParticleVariantColor = ParticleVariantColor;
    PSC->bIsFirstPerson = bIsFirstPerson;
    return PSC;
}

}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAtLocation(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector SpawnLocation,
                                                                   FRotator SpawnRotation, bool bAutoDestroy, EPSCPoolMethod PoolingMethod,
                                                                   bool bAutoActivateSystem) {
    return SpawnEmitterAtLocation(WorldContextObject, EmitterTemplate, SpawnLocation, SpawnRotation, FVector(1.f), bAutoDestroy, PoolingMethod,
                                  bAutoActivateSystem, false, 0);
}

UParticleSystemComponent* UGameplayStatics::InternalSpawnEmitterAtLocation(UWorld* World, UParticleSystem* EmitterTemplate, FVector SpawnLocation,
                                                                           FRotator SpawnRotation, FVector SpawnScale, bool bAutoDestroy,
                                                                           EPSCPoolMethod PoolingMethod, bool bAutoActivateSystem, bool bIsFirstPerson) {
    UParticleSystemComponent* PSC = CreateParticleSystem(EmitterTemplate, World, World->GetWorldSettings(), bAutoDestroy, PoolingMethod, bIsFirstPerson, 0);
    if (PSC == nullptr)
        return nullptr;
    PSC->SetUsingAbsoluteLocation(true);
    PSC->SetUsingAbsoluteRotation(true);
    PSC->SetUsingAbsoluteScale(true);
    PSC->SetRelativeLocation_Direct(SpawnLocation);
    PSC->SetRelativeRotation_Direct(SpawnRotation);
    PSC->SetRelativeScale3D_Direct(SpawnScale);
    PSC->RegisterComponentWithWorld(World);
    if (bAutoActivateSystem)
        PSC->ActivateSystem(true);
    return PSC;
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAtLocation(const UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector SpawnLocation,
                                                                   FRotator SpawnRotation, FVector SpawnScale, bool bAutoDestroy, EPSCPoolMethod PoolingMethod,
                                                                   bool bAutoActivateSystem, bool bIsFirstPerson, float) {
    UParticleSystemComponent* PSC = nullptr;
    if (EmitterTemplate)
        if (UWorld* World = GetWorldFromContextObject(WorldContextObject))
            PSC = InternalSpawnEmitterAtLocation(World, EmitterTemplate, SpawnLocation, SpawnRotation, SpawnScale, bAutoDestroy, PoolingMethod,
                                                 bAutoActivateSystem, bIsFirstPerson);
    return PSC;
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAtLocation(UWorld* World, UParticleSystem* EmitterTemplate, const FTransform& SpawnTransform,
                                                                   bool bAutoDestroy, EPSCPoolMethod PoolingMethod, bool bAutoActivateSystem) {
    UParticleSystemComponent* PSC = nullptr;
    if (World && EmitterTemplate)
        PSC = InternalSpawnEmitterAtLocation(World, EmitterTemplate, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(),
                                             SpawnTransform.GetScale3D(), bAutoDestroy, PoolingMethod, bAutoActivateSystem, false);
    return PSC;
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAttached(UParticleSystem* EmitterTemplate, USceneComponent* AttachToComponent, FName AttachPointName,
                                                                 FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bAutoDestroy,
                                                                 EPSCPoolMethod PoolingMethod, bool bAutoActivateSystem) {
    return SpawnEmitterAttached(EmitterTemplate, AttachToComponent, AttachPointName, Location, Rotation, FVector(1.f), LocationType, bAutoDestroy,
                                PoolingMethod, bAutoActivateSystem);
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAttached(UParticleSystem* EmitterTemplate, USceneComponent* AttachToComponent, FName AttachPointName,
                                                                 FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType,
                                                                 bool bAutoDestroy, EPSCPoolMethod PoolingMethod, bool bAutoActivateSystem, bool bIsFirstPerson,
                                                                 float WarmupTime, uint8 ParticleVariantColor) {
    if (EmitterTemplate == nullptr || AttachToComponent == nullptr)
        return nullptr;
    UWorld* const World = AttachToComponent->GetWorld();
    if (GetRuntimeProvider().getGame() == EGame::GAME_Valorant)
        if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(AttachToComponent))
            bIsFirstPerson = Primitive->bIsFirstPerson;
    UParticleSystemComponent* PSC =
        CreateParticleSystem(EmitterTemplate, World, AttachToComponent->GetOwner(), bAutoDestroy, PoolingMethod, bIsFirstPerson, ParticleVariantColor);
    if (PSC == nullptr)
        return nullptr;
    PSC->SetupAttachment(AttachToComponent, AttachPointName);
    if (LocationType == EAttachLocation::KeepWorldPosition) {
        const FTransform ParentToWorld = AttachToComponent->GetSocketTransform(AttachPointName);
        const FTransform ComponentToWorld(Rotation, Location, Scale);
        const FTransform RelativeTM = ComponentToWorld.GetRelativeTransform(ParentToWorld);
        PSC->SetRelativeLocation_Direct(RelativeTM.GetLocation());
        PSC->SetRelativeRotation_Direct(RelativeTM.GetRotation().Rotator());
        PSC->SetRelativeScale3D_Direct(RelativeTM.GetScale3D());
    } else {
        PSC->SetRelativeLocation_Direct(Location);
        PSC->SetRelativeRotation_Direct(Rotation);
        if (LocationType == EAttachLocation::SnapToTarget) {
            const FTransform ParentToWorld = AttachToComponent->GetSocketTransform(AttachPointName);
            PSC->SetRelativeScale3D_Direct(Scale * ParentToWorld.GetSafeScaleReciprocal(ParentToWorld.GetScale3D()));
        } else
            PSC->SetRelativeScale3D_Direct(Scale);
    }
    PSC->AdditionalWarmupTime += WarmupTime;
    PSC->RegisterComponentWithWorld(World);
    if (bAutoActivateSystem)
        PSC->ActivateSystem(true);
    return PSC;
}

}
