#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PaperFlipbookActor.generated.h"

namespace ue5parse::runtime {

class UPaperFlipbookComponent;

UCLASS()
class APaperFlipbookActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperFlipbookComponent> RenderComponent = nullptr;

    explicit APaperFlipbookActor(const FObjectInitializer& ObjectInitializer);
};

}
