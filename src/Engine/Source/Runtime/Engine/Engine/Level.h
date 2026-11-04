#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Level.generated.h"

namespace ue5parse::runtime {

class UWorld;

UCLASS()
class ULevel : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<AActor> WorldSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UWorld> OwningWorld = nullptr;

    TArray<AActor*> Actors;
    FURL URL;
    UObject* Model = nullptr;
    TArray<UObject*> ModelComponents;
    UObject* LevelScriptActor = nullptr;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
};

}
