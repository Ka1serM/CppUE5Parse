#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "PaperTileMapRenderComponent.generated.h"

namespace ue5parse::runtime {

class UPaperTileMap;

UCLASS()
class UPaperTileMapRenderComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperTileMap> TileMap = nullptr;

    explicit UPaperTileMapRenderComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
