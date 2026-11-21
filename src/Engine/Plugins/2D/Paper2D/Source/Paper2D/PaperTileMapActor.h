#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PaperTileMapActor.generated.h"

namespace ue5parse::runtime {

class UPrimitiveComponent;

UCLASS()
class APaperTileMapActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> RenderComponent = nullptr;

    explicit APaperTileMapActor(const FObjectInitializer& ObjectInitializer);
};

}
