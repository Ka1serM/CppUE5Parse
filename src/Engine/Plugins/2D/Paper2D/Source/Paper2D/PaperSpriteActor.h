#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PaperSpriteActor.generated.h"

namespace ue5parse::runtime {

class UPaperSpriteComponent;

UCLASS()
class APaperSpriteActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperSpriteComponent> RenderComponent = nullptr;

    explicit APaperSpriteActor(const FObjectInitializer& ObjectInitializer);
};

}
