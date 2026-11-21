#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/HairCardsDatas.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FPackedNormal& Normal) {
    return Ar << Normal.Packed;
}

FArchive& operator<<(FArchive& Ar, FHairCardsInterpolationVertex& Vertex) {
    uint32 Value = 0;
    Ar << Value;
    Vertex.VertexIndex = Value & 0x00FFFFFF;
    Vertex.VertexLerp = Value >> 24;
    return Ar;
}

void FHairCardsBulkData::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    Ar << Positions << Normals << UVs << Materials;
    if (Game >= EGame::GAME_UE5_6)
        Ar << VertexColors;
    Ar << Indices << BoundingBox;
    if (Game >= EGame::GAME_UE5_7)
        Ar << NumUVs;
    if (Game >= EGame::GAME_UE5_6)
        Ar << bVertexColor;
}

void FHairCardsInterpolationBulkData::Serialize(FArchive& Ar) {
    Ar << Interpolation;
}

void FHairMeshesBulkData::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    Ar << Positions << Normals << UVs;
    if (Game >= EGame::GAME_UE5_6)
        Ar << VertexColors;
    Ar << Indices << BoundingBox;
    if (Game >= EGame::GAME_UE5_7)
        Ar << NumUVs;
    if (Game >= EGame::GAME_UE5_6)
        Ar << bVertexColor;
}

}
