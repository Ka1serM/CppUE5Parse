#pragma once

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/IntMargin.h"
#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/SpriteEditorOnlyTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PaperTileSet.generated.h"

namespace ue5parse::runtime {

class UTexture;
class UTexture2D;

USTRUCT()
struct FPaperTileMetadata {
    GENERATED_BODY()
    UPROPERTY()
    FName UserDataName;
    UPROPERTY()
    FSpriteGeometryCollection CollisionData = {.GeometryType = ESpritePolygonMode::FullyCustom};
    UPROPERTY()
    uint8 TerrainMembership[4] = {0xFF, 0xFF, 0xFF, 0xFF};
};

USTRUCT()
struct FPaperTileSetTerrain {
    GENERATED_BODY()
    UPROPERTY()
    FString TerrainName;
    UPROPERTY()
    int32 CenterTileIndex = 0;
};

UCLASS()
class UPaperTileSet : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FIntPoint TileSize = FIntPoint(32, 32);
    UPROPERTY()
    TObjectPtr<UTexture2D> TileSheet = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UTexture>> AdditionalSourceTextures;
    UPROPERTY()
    FIntMargin BorderMargin;
    UPROPERTY()
    FIntPoint PerTileSpacing = FIntPoint(0, 0);
    UPROPERTY()
    FIntPoint DrawingOffset = FIntPoint(0, 0);
    UPROPERTY()
    int32 WidthInTiles = 0;
    UPROPERTY()
    int32 HeightInTiles = 0;
    UPROPERTY()
    int32 AllocatedWidth = 0;
    UPROPERTY()
    int32 AllocatedHeight = 0;
    UPROPERTY()
    TArray<FPaperTileMetadata> PerTileData;
    UPROPERTY()
    TArray<FPaperTileSetTerrain> Terrains;
    UPROPERTY()
    int32 TileWidth_DEPRECATED = 32;
    UPROPERTY()
    int32 TileHeight_DEPRECATED = 32;
    UPROPERTY()
    int32 Margin_DEPRECATED = 0;
    UPROPERTY()
    int32 Spacing_DEPRECATED = 0;
};

}
