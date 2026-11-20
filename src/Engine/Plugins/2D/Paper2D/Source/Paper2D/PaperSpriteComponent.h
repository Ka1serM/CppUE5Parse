#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PaperSpriteComponent.generated.h"

namespace ue5parse::runtime {

class UPaperSprite;

UCLASS()
class UPaperSpriteComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperSprite> SourceSprite = nullptr;

    explicit UPaperSpriteComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
