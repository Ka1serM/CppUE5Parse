#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "RuntimePartition.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ERuntimePartitionCellBoundsMethod : uint8 {
    UseContent,
    UseCellBounds,
    UseMinContentCellBounds,
};

UCLASS(Abstract)
class URuntimePartition : public UObject {
    GENERATED_BODY()
public:
    explicit URuntimePartition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FName Name;
    UPROPERTY()
    bool bBlockOnSlowStreaming = false;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    ERuntimePartitionCellBoundsMethod BoundsMethod = ERuntimePartitionCellBoundsMethod::UseMinContentCellBounds;
    UPROPERTY()
    bool bIsHLODSetup = false;
    UPROPERTY()
    int32 LoadingRange = 25600;
    UPROPERTY()
    FLinearColor DebugColor = FLinearColor::Black;
    UPROPERTY()
    int32 HLODIndex = 0;
};

UCLASS()
class URuntimePartitionLHGrid : public URuntimePartition {
    GENERATED_BODY()
public:
    explicit URuntimePartitionLHGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    uint32 CellSize = 12800;
    UPROPERTY()
    FVector Origin = FVector::ZeroVector;
    UPROPERTY()
    bool bIs2D = true;
};

UCLASS()
class URuntimePartitionPersistent : public URuntimePartition {
    GENERATED_BODY()
public:
    explicit URuntimePartitionPersistent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
