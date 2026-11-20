#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "TextRenderActor.generated.h"

namespace ue5parse::runtime {

class UTextRenderComponent;

UCLASS()
class ATextRenderActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> TextRender = nullptr;

    explicit ATextRenderActor(const FObjectInitializer& ObjectInitializer);
};

}
