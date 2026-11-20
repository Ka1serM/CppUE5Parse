#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PaperTileMapComponent.generated.h"

namespace ue5parse::runtime {

class UPaperTileMap;

UCLASS()
class UPaperTileMapComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperTileMap> TileMap = nullptr;

    explicit UPaperTileMapComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
