#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "WorldPartitionRuntimeSpatialHash.generated.h"

namespace ue5parse::runtime {

class UHLODLayer;

USTRUCT()
struct FSpatialHashRuntimeGrid {
    GENERATED_BODY()
    UPROPERTY()
    FName GridName;
    UPROPERTY()
    int32 CellSize = 12800;
    UPROPERTY()
    float LoadingRange = 25600.0f;
    UPROPERTY()
    bool bBlockOnSlowStreaming = false;
    UPROPERTY()
    FVector2D Origin = FVector2D::ZeroVector;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    FLinearColor DebugColor;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    TObjectPtr<UHLODLayer> HLODLayer = nullptr;
};

UCLASS()
class ASpatialHashRuntimeGridInfo : public AInfo {
    GENERATED_BODY()
public:
    explicit ASpatialHashRuntimeGridInfo(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FSpatialHashRuntimeGrid GridSettings;
};

}
