#include "Engine/Source/Runtime/Engine/Model.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FBspSurf& Surf) {
    Ar << Surf.Material << Surf.PolyFlags << Surf.pBase << Surf.vNormal << Surf.vTextureU << Surf.vTextureV << Surf.iBrushPoly;
    return Ar << Surf.Actor << Surf.Plane << Surf.LightMapScale << Surf.iLightmassIndex;
}

FArchive& operator<<(FArchive& Ar, FModelVertex& Vertex) {
    return Ar << Vertex.Position << Vertex.TangentX << Vertex.TangentZ << Vertex.TexCoord << Vertex.ShadowTexCoord;
}

FArchive& operator<<(FArchive& Ar, FModelVertexBuffer& Buffer) {
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::IncreaseNormalPrecision, EGame::GAME_UE4_20))
        return Ar << Buffer.Vertices;
    TArray<FDepecatedModelVertex> DeprecatedVertices;
    BulkSerialize(Ar, DeprecatedVertices);
    Buffer.Vertices.Empty(DeprecatedVertices.Num());
    for (const FDepecatedModelVertex& Deprecated : DeprecatedVertices) {
        const FVector4f TangentX = Deprecated.TangentX.ToFVector4f();
        Buffer.Vertices.Add({Deprecated.Position, FVector3f(TangentX.X, TangentX.Y, TangentX.Z), Deprecated.TangentZ.ToFVector4f(),
                             Deprecated.TexCoord, Deprecated.ShadowTexCoord});
    }
    return Ar;
}

namespace {

struct FZoneProperties {
    UObject* ZoneActor = nullptr;
    uint64 Connectivity = 0;
    uint64 Visibility = 0;
    float LastRenderTime = 0;
};

void SkipZones(FArchive& Ar) {
    int32 NumZones = 0;
    Ar << NumZones;
    for (int32 i = 0; i < NumZones && !Ar.IsError(); i++) {
        FZoneProperties Zone;
        Ar << Zone.ZoneActor << Zone.Connectivity << Zone.Visibility << Zone.LastRenderTime;
    }
}

}

void UModel::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    constexpr uint8 StripVertexBufferFlag = 1;
    FStripDataFlags StripFlags(Ar);
    Ar << Bounds;
    BulkSerialize(Ar, Vectors);
    BulkSerialize(Ar, Points);
    BulkSerialize(Ar, Nodes);
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::BSP_UNDO_FIX) {
        UObject* SurfsOwner = nullptr;
        Ar << SurfsOwner;
    }
    Ar << Surfs;
    BulkSerialize(Ar, Verts);
    Ar << NumSharedSides;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::REMOVE_ZONES_FROM_MODEL)
        SkipZones(Ar);
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::REMOVE_UNUSED_UPOLYS_FROM_UMODEL) {
        UObject* Polys = nullptr;
        Ar << Polys;
        SkipBulkSerializedArray(Ar);
        SkipBulkSerializedArray(Ar);
    }
    Ar << RootOutside << Linked;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::REMOVE_ZONES_FROM_MODEL)
        SkipBulkSerializedArray(Ar);
    Ar << NumUniqueVertices;
    if (!StripFlags.IsEditorDataStripped() || !StripFlags.IsClassDataStripped(StripVertexBufferFlag))
        Ar << VertexBuffer;
    Ar << LightingGuid << LightmassSettings;
}

}
