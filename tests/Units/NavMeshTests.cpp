#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastVersion.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FMemoryReaderView;
using ue5parse::runtime::FPImplRecastNavMesh;
using ue5parse::runtime::FRecastMeshTile;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    template <typename T>
    void putAll(std::initializer_list<T> values) {
        for (T value : values)
            put<T>(value);
    }

    std::vector<std::byte> bytes;
};

struct TileLayout {
    std::uint32_t navMeshVersion = ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT;
    bool wideLinkIds = true;
    bool mongilStarDive = false;
    std::int32_t tileCacheSize = 3;
};

void putTile(Writer& w, const TileLayout& layout) {
    const bool tileVertEncoding = layout.navMeshVersion >= ue5parse::runtime::NAVMESHVER_TILEVERT_ENCODING;
    w.putAll<std::uint16_t>({2, 1, 0, 1, 1, 1, 1, 1, 0, 1});
    w.put<std::uint16_t>(7);
    w.putAll<std::int32_t>({4, -5});
    w.putAll<std::uint16_t>({0, 1, 2, 0, 1, 1, 1, 1, 1, 1});
    if (layout.navMeshVersion >= ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS)
        w.put<std::uint8_t>(2);
    w.putAll<double>({1000.0, 2000.0, -300.0, 1500.0, 2500.0, 100.0});
    w.putAll<std::uint16_t>({1, 0, 0, 0});
    if (tileVertEncoding) {
        w.putAll<float>({10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f});
    } else {
        w.putAll<double>({1010.0, 2020.0, -270.0, 1040.0, 2050.0, -240.0});
    }
    w.put<std::uint32_t>(0);
    w.putAll<std::uint16_t>({0, 1, 0, 0, 0, 0, 0xFFFF, 0xFFFF, 0, 0, 0, 0, 0x0003});
    w.putAll<std::uint8_t>({2, 0x41});
    w.putAll<std::uint16_t>({0, 0});
    w.putAll<std::uint8_t>({1, 1});
    if (tileVertEncoding)
        w.putAll<float>({5.0f, 6.0f, 7.0f});
    else
        w.putAll<double>({1005.0, 2006.0, -293.0});
    w.putAll<std::uint8_t>({0, 1, 2, 3});
    if (layout.mongilStarDive)
        w.put<std::uint8_t>(0xEE);
    w.putAll<std::uint16_t>({1, 2, 3, 4, 5, 6});
    w.put<std::int32_t>(-1);
    w.putAll<double>({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 25.0});
    w.put<std::uint16_t>(0);
    w.putAll<std::uint8_t>({0x80, 1});
    if (layout.wideLinkIds)
        w.put<std::uint64_t>(0x123456789ABCull);
    else
        w.put<std::uint32_t>(0x89ABCDEFu);
    w.put<double>(-1.0);
    w.putAll<double>({1100.0, 2200.0, -100.0});
    w.put<std::uint16_t>(0);
    w.put<std::int32_t>(layout.tileCacheSize);
    if (layout.tileCacheSize < 0)
        return;
    w.put<std::int16_t>(1);
    w.putAll<std::int32_t>({4, -5, 0});
    w.putAll<double>({1000.0, 1500.0, 2000.0, 2500.0, -300.0, 100.0});
    w.putAll<std::uint16_t>({1, 2, 64, 65, 0, 63, 0, 64});
    for (std::int32_t index = 0; index < layout.tileCacheSize; index++)
        w.put<std::uint8_t>(static_cast<std::uint8_t>(0xA0 + index));
}

void putNavMesh(Writer& w, const TileLayout& layout) {
    w.put<std::int32_t>(2);
    w.putAll<double>({-100.0, -200.0, -300.0, 1600.0, 1600.0});
    w.putAll<std::int32_t>({64, 4096});
    if (layout.navMeshVersion >= ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS)
        w.putAll<double>({144.0, 34.0, 35.0, 0.01, 0.02, 0.04});
    else
        w.putAll<double>({144.0, 34.0, 35.0, 0.05});
    w.put<std::uint64_t>(~0ull);
    w.put<std::int32_t>(0);
    w.put<std::uint64_t>(0x10001);
    w.put<std::int32_t>(512);
    putTile(w, layout);
}

FPImplRecastNavMesh readNavMesh(const std::vector<std::byte>& bytes, EGame game, std::uint32_t navMeshVersion) {
    FMemoryReaderView Ar(bytes);
    Ar.SetGame(game);
    FPImplRecastNavMesh NavMesh(nullptr);
    NavMesh.Serialize(Ar, navMeshVersion);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    return NavMesh;
}

void checkTileBody(const FRecastMeshTile& Tile) {
    CHECK(Tile.Header.version == 7);
    CHECK(Tile.Header.x == 4);
    CHECK(Tile.Header.y == -5);
    CHECK(Tile.Header.polyCount == 1);
    CHECK(Tile.Header.vertCount == 2);
    CHECK(Tile.Header.bmin[2] == -300.0);
    CHECK(Tile.Header.bmax[0] == 1500.0);
    CHECK(Tile.Header.clusterCount == 1);
    REQUIRE(Tile.NavVerts.Num() == 2);
    CHECK(Tile.NavVerts[0].X == 1010.0);
    CHECK(Tile.NavVerts[0].Z == -270.0);
    CHECK(Tile.NavVerts[1].Y == 2050.0);
    REQUIRE(Tile.NavPolys.Num() == 1);
    CHECK(Tile.NavPolys[0].verts[1] == 1);
    CHECK(Tile.NavPolys[0].neis[0] == 0xFFFF);
    CHECK(Tile.NavPolys[0].flags == 3);
    CHECK(Tile.NavPolys[0].vertCount == 2);
    CHECK(Tile.NavPolys[0].areaAndtype == 0x41);
    REQUIRE(Tile.DetailMeshes.Num() == 1);
    CHECK(Tile.DetailMeshes[0].vertCount == 1);
    REQUIRE(Tile.DetailVerts.Num() == 1);
    CHECK(Tile.DetailVerts[0].Y == 2006.0);
    REQUIRE(Tile.DetailTris.Num() == 4);
    CHECK(Tile.DetailTris[3] == 3);
    REQUIRE(Tile.BVTree.Num() == 1);
    CHECK(Tile.BVTree[0].bmax[2] == 6);
    CHECK(Tile.BVTree[0].i == -1);
    REQUIRE(Tile.OffMeshCons.Num() == 1);
    CHECK(Tile.OffMeshCons[0].pos[5] == 6.0);
    CHECK(Tile.OffMeshCons[0].rad == 25.0);
    CHECK(Tile.OffMeshCons[0].flags == 0x80);
    CHECK(Tile.OffMeshCons[0].side == 1);
    CHECK(Tile.OffMeshCons[0].height == -1.0);
    CHECK(Tile.OffMeshSegs.Num() == 0);
    REQUIRE(Tile.Clusters.Num() == 1);
    CHECK(Tile.Clusters[0].center[1] == 2200.0);
    REQUIRE(Tile.PolyClusters.Num() == 1);
    CHECK(Tile.ClusterCons.Num() == 0);
}

}

TEST(recastNavMeshReadsLargeWorldTilesAndTileCacheLayer) {
    Writer w;
    putNavMesh(w, TileLayout{});
    FPImplRecastNavMesh NavMesh = readNavMesh(w.bytes, EGame::GAME_UE5_3, ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT);
    CHECK(NavMesh.GetParams().orig[1] == -200.0);
    CHECK(NavMesh.GetParams().tileWidth == 1600.0);
    CHECK(NavMesh.GetParams().maxPolys == 4096);
    CHECK(NavMesh.GetParams().walkableClimb == 35.0);
    CHECK(NavMesh.GetParams().resolutionParams[0].bvQuantFactor == 0.01);
    CHECK(NavMesh.GetParams().resolutionParams[2].bvQuantFactor == 0.04);
    REQUIRE(NavMesh.GetTiles().Num() == 1);
    const FRecastMeshTile& Tile = NavMesh.GetTiles()[0];
    CHECK(Tile.TileRef == 0x10001);
    CHECK(Tile.Header.resolution == 2);
    checkTileBody(Tile);
    CHECK(Tile.OffMeshCons[0].userId == 0x123456789ABCull);
    REQUIRE(Tile.TileCacheLayerHeader.has_value());
    CHECK(Tile.TileCacheLayerHeader->ty == -5);
    CHECK(Tile.TileCacheLayerHeader->bmax[0] == 1500.0);
    CHECK(Tile.TileCacheLayerHeader->bmin[2] == -300.0);
    CHECK(Tile.TileCacheLayerHeader->width == 64);
    CHECK(Tile.TileCacheLayerHeader->maxy == 64);
    REQUIRE(Tile.TileCacheCompressedData.Num() == 3);
    CHECK(Tile.TileCacheCompressedData[2] == 0xA2);
}

TEST(recastNavMeshStoresTileVertsRelativeToTileBoundsSinceTileVertEncoding) {
    Writer w;
    TileLayout layout;
    layout.navMeshVersion = ue5parse::runtime::NAVMESHVER_TILEVERT_ENCODING;
    layout.tileCacheSize = -1;
    putNavMesh(w, layout);
    FPImplRecastNavMesh NavMesh = readNavMesh(w.bytes, EGame::GAME_UE5_8, ue5parse::runtime::NAVMESHVER_TILEVERT_ENCODING);
    REQUIRE(NavMesh.GetTiles().Num() == 1);
    const FRecastMeshTile& Tile = NavMesh.GetTiles()[0];
    checkTileBody(Tile);
    CHECK(!Tile.TileCacheLayerHeader.has_value());
}

TEST(recastNavMeshReadsNarrowLinkIdsBeforeUE5_3) {
    Writer w;
    TileLayout layout;
    layout.navMeshVersion = ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS_CELLHEIGHT;
    layout.wideLinkIds = false;
    layout.tileCacheSize = 0;
    putNavMesh(w, layout);
    FPImplRecastNavMesh NavMesh = readNavMesh(w.bytes, EGame::GAME_UE5_2, ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS_CELLHEIGHT);
    REQUIRE(NavMesh.GetTiles().Num() == 1);
    const FRecastMeshTile& Tile = NavMesh.GetTiles()[0];
    checkTileBody(Tile);
    CHECK(Tile.OffMeshCons[0].userId == 0x89ABCDEFu);
    REQUIRE(Tile.TileCacheLayerHeader.has_value());
    CHECK(Tile.TileCacheCompressedData.Num() == 0);
}

TEST(recastNavMeshCopiesDefaultQuantFactorBeforeTileResolutions) {
    Writer w;
    TileLayout layout;
    layout.navMeshVersion = ue5parse::runtime::NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS;
    layout.wideLinkIds = false;
    putNavMesh(w, layout);
    FPImplRecastNavMesh NavMesh = readNavMesh(w.bytes, EGame::GAME_UE5_0, ue5parse::runtime::NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS);
    CHECK(NavMesh.GetParams().walkableHeight == 144.0);
    CHECK(NavMesh.GetParams().resolutionParams[0].bvQuantFactor == 0.05);
    CHECK(NavMesh.GetParams().resolutionParams[2].bvQuantFactor == 0.05);
    REQUIRE(NavMesh.GetTiles().Num() == 1);
    CHECK(NavMesh.GetTiles()[0].Header.resolution == 0);
    checkTileBody(NavMesh.GetTiles()[0]);
}

TEST(mongilStarDiveRecastTileSkipsOneBytePerPolyAfterDetailTris) {
    Writer w;
    TileLayout layout;
    layout.mongilStarDive = true;
    putTile(w, layout);
    FMemoryReaderView Ar(w.bytes);
    Ar.SetGame(EGame::GAME_MongilStarDive);
    FRecastMeshTile Tile;
    FPImplRecastNavMesh::SerializeRecastMeshTile(Ar, ue5parse::runtime::NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT, Tile);
    FPImplRecastNavMesh::SerializeCompressedTileCacheData(Ar, Tile);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    checkTileBody(Tile);
}

TEST(recastNavMeshVersionRangeFollowsEngine) {
    using ue5parse::runtime::IsCompatibleNavMeshVersion;
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE5_0, 19));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_0, 20));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_1, 21));
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE5_1, 22));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_2, 24));
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE5_2, 25));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_6, 26));
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE5_6, 27));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_7, 27));
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE5_7, 28));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE5_8, 28));
    CHECK(!IsCompatibleNavMeshVersion(EGame::GAME_UE4_4, 9));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE4_4, 10));
    CHECK(IsCompatibleNavMeshVersion(EGame::GAME_UE4_3, 9));
}
