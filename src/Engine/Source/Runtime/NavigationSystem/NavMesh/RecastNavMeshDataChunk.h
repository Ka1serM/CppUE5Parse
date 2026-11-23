#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationDataChunk.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"

#include "RecastNavMeshDataChunk.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URecastNavMeshDataChunk : public UNavigationDataChunk {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    const TArray<FRecastMeshTile>& GetTiles() const { return Tiles; }

private:
    void SerializeRecastData(FArchive& Ar, uint32 NavMeshVersion);

    TArray<FRecastMeshTile> Tiles;
};

}
