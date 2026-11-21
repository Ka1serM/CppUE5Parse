#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DatasmithSceneActor.generated.h"

namespace ue5parse::runtime {

class UDatasmithScene;

UCLASS()
class ADatasmithSceneActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADatasmithSceneActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UDatasmithScene> Scene = nullptr;
    UPROPERTY()
    TMap<FName, TSoftObjectPtr<AActor>> RelatedActors;
};

}
