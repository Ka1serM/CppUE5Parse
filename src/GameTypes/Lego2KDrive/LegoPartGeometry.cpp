#include "GameTypes/Lego2KDrive/LegoPartGeometry.h"

namespace ue5parse::runtime {

bool FLegoPartLODGeometry::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
        return false;
    Ar << bUsingFallback << bHasUVs << bGeneratedTangents;
    BulkSerialize(Ar, Positions);
    BulkSerialize(Ar, Tangents);
    if (bHasUVs)
        BulkSerialize(Ar, UVs);
    BulkSerialize(Ar, Indices);
    return true;
}

void ULegoPartGeometry::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    BulkSerialize(Ar, Buffer);
    Ar << Size << Box.Min << Box.Max << Box.IsValid << SomeVector << SomeSize;
}

}
