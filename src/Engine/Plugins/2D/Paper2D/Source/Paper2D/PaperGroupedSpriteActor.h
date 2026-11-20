#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PaperGroupedSpriteActor.generated.h"

namespace ue5parse::runtime {

class UPaperGroupedSpriteComponent;

UCLASS()
class APaperGroupedSpriteActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperGroupedSpriteComponent> RenderComponent = nullptr;

    explicit APaperGroupedSpriteActor(const FObjectInitializer& ObjectInitializer);
};

}
