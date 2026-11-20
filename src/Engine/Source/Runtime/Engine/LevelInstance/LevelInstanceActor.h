#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LevelInstanceActor.generated.h"

namespace ue5parse::runtime {

class ULevelInstanceComponent;
class UWorld;

UCLASS()
class ALevelInstance : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<ULevelInstanceComponent> LevelInstanceComponent = nullptr;
    UPROPERTY()
    TSoftObjectPtr<UWorld> CookedWorldAsset;

    explicit ALevelInstance(const FObjectInitializer& ObjectInitializer);

    void Serialize(FArchive& Ar) override;

private:
    FGuid LevelInstanceActorGuid;
};

}
