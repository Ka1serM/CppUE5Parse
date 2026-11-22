#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FWorldTileLayer {
public:
    friend FArchive& operator<<(FArchive& Ar, FWorldTileLayer& D);

    FString Name = FString("Uncategorized");
    int32 Reserved0 = 0;
    FIntPoint Reserved1;
    int32 StreamingDistance = 50000;
    bool DistanceStreamingEnabled = true;
};

class FWorldTileLODInfo {
public:
    friend FArchive& operator<<(FArchive& Ar, FWorldTileLODInfo& D);

    int32 RelativeStreamingDistance = 10000;
    float Reserved0 = 0;
    float Reserved1 = 0;
    int32 Reserved2 = 0;
    int32 Reserved3 = 0;
};

class FWorldTileInfo {
public:
    friend FArchive& operator<<(FArchive& Ar, FWorldTileInfo& D) {
        D.StreamArchive(Ar);
        return Ar;
    }

    FIntVector Position;
    FIntVector AbsolutePosition;
    FBox Bounds;
    FWorldTileLayer Layer;
    bool bHideInTileView = false;
    FString ParentTilePackageName;
    TArray<FWorldTileLODInfo> LODList;
    int32 ZOrder = 0;

private:
    void StreamArchive(FArchive& Ar);
};

}
