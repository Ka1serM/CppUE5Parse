#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PaperTileLayer.generated.h"

namespace ue5parse::runtime {

class UPaperTileSet;

USTRUCT()
struct FPaperTileInfo {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UPaperTileSet> TileSet = nullptr;
    UPROPERTY()
    int32 PackedTileIndex = INDEX_NONE;
};

UCLASS()
class UPaperTileLayer : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FText LayerName;
    UPROPERTY()
    int32 LayerWidth = 4;
    UPROPERTY()
    int32 LayerHeight = 4;
    UPROPERTY()
    uint32 bHiddenInGame : 1 = false;
    UPROPERTY()
    uint32 bLayerCollides : 1 = true;
    UPROPERTY()
    uint32 bOverrideCollisionThickness : 1 = false;
    UPROPERTY()
    uint32 bOverrideCollisionOffset : 1 = false;
    UPROPERTY()
    float CollisionThicknessOverride = 50.0f;
    UPROPERTY()
    float CollisionOffsetOverride = 0.0f;
    UPROPERTY()
    FLinearColor LayerColor = FLinearColor::White;
    UPROPERTY()
    int32 AllocatedWidth = 0;
    UPROPERTY()
    int32 AllocatedHeight = 0;
    UPROPERTY()
    TArray<FPaperTileInfo> AllocatedCells;
    UPROPERTY()
    TObjectPtr<UPaperTileSet> TileSet_DEPRECATED = nullptr;
    UPROPERTY()
    TArray<int32> AllocatedGrid_DEPRECATED;
};

}
