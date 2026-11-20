#pragma once

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/SpriteEditorOnlyTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "PaperTileMap.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;
class UPaperTileLayer;
class UPaperTileSet;

namespace ETileMapProjectionMode {

UENUM()
enum Type : int32 { Orthogonal, IsometricDiamond, IsometricStaggered, HexagonalStaggered };

}

UCLASS()
class UPaperTileMap : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 MapWidth = 4;
    UPROPERTY()
    int32 MapHeight = 4;
    UPROPERTY()
    int32 TileWidth = 32;
    UPROPERTY()
    int32 TileHeight = 32;
    UPROPERTY()
    float PixelsPerUnrealUnit = 1.0f;
    UPROPERTY()
    float SeparationPerTileX = 0.0f;
    UPROPERTY()
    float SeparationPerTileY = 0.0f;
    UPROPERTY()
    float SeparationPerLayer = 4.0f;
    UPROPERTY()
    TSoftObjectPtr<UPaperTileSet> SelectedTileSet;
    UPROPERTY()
    TObjectPtr<UPaperTileSet> DefaultLayerTileSet = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UPaperTileLayer>> TileLayers;
    UPROPERTY()
    float CollisionThickness = 50.0f;
    UPROPERTY()
    TEnumAsByte<ESpriteCollisionMode::Type> SpriteCollisionDomain = ESpriteCollisionMode::Use3DPhysics;
    UPROPERTY()
    TEnumAsByte<ETileMapProjectionMode::Type> ProjectionMode = ETileMapProjectionMode::Orthogonal;
    UPROPERTY()
    int32 HexSideLength = 0;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    int32 LayerNameIndex = 0;
};

}
