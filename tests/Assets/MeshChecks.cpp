#include <cstdlib>
#include <span>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAssetCommon.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace test::assets {

namespace {

std::span<const Json> getItems(const Json& value) {
    if (value.isArray())
        return value.getArray();
    return {&value, 1};
}

template <class TMaterial>
void checkMaterialSlots(const std::string& path, const rt::TArray<TMaterial>& materials, const Json& value) {
    std::span<const Json> expected = getItems(value);
    if (!CHECK(materials.Num() == static_cast<rt::int32>(expected.size())))
        return;
    for (rt::int32 i = 0; i < materials.Num(); i++)
        checkText(getPathName(materials[i].MaterialInterface.Get()), expected[i].getString(), describe(path, "material " + std::to_string(i)));
}

void checkLodCount(const std::string& path, const Json& value) {
    if (rt::UStaticMesh* mesh = loadObject<rt::UStaticMesh>(path))
        checkNear(mesh->GetNumLODs(), value.getNumber(), 0.0, describe(path, "LODs"));
}

void checkLods(const std::string& path, const Json& value) {
    rt::UStaticMesh* mesh = loadObject<rt::UStaticMesh>(path);
    if (mesh == nullptr || !CHECK(mesh->GetRenderData() != nullptr))
        return;
    const rt::TArray<rt::FStaticMeshLODResources>& lods = mesh->GetRenderData()->LODResources;
    for (const auto& [index, counts] : value.getObject()) {
        rt::int32 lod = std::atoi(index.c_str());
        if (lod >= lods.Num()) {
            fail(describe(path, "LOD " + index), "is missing");
            continue;
        }
        std::string what = describe(path, "LOD " + index);
        checkNear(lods[lod].Sections.Num(), counts.find("sections")->getNumber(), 0.0, what + " sections");
        if (const Json* vertices = counts.find("vertices"))
            checkNear(lods[lod].GetNumVertices(), vertices->getNumber(), 0.0, what + " vertices");
        if (const Json* triangles = counts.find("triangles"))
            checkNear(lods[lod].IndexBuffer.GetNumIndices() / 3, triangles->getNumber(), 0.0, what + " triangles");
    }
}

void checkBoundsOfStaticMesh(const std::string& path, const Json& value) {
    if (rt::UStaticMesh* mesh = loadObject<rt::UStaticMesh>(path))
        checkBounds(mesh->GetBounds(), value, path);
}

void checkNanite(const std::string& path, const Json& value) {
    rt::UStaticMesh* mesh = loadObject<rt::UStaticMesh>(path);
    if (mesh == nullptr)
        return;
    double counts[] = {static_cast<double>(mesh->GetNumNaniteVertices()), static_cast<double>(mesh->GetNumNaniteTriangles())};
    checkNumbers(counts, value, 0.0, describe(path, "Nanite vertices and triangles"));
}

void checkMaterials(const std::string& path, const Json& value) {
    rt::UStaticMesh* mesh = loadObject<rt::UStaticMesh>(path);
    if (mesh == nullptr)
        return;
    checkMaterialSlots(path, mesh->GetStaticMaterials(), value);
}

void checkBoundsOfSkeletalMesh(const std::string& path, const Json& value) {
    if (rt::USkeletalMesh* mesh = loadObject<rt::USkeletalMesh>(path))
        checkBounds(mesh->GetBounds(), value, path);
}

void checkSkeletalMaterials(const std::string& path, const Json& value) {
    rt::USkeletalMesh* mesh = loadObject<rt::USkeletalMesh>(path);
    if (mesh == nullptr)
        return;
    checkMaterialSlots(path, mesh->GetMaterials(), value);
}

void checkSkeleton(const std::string& path, const Json& value) {
    if (rt::USkeletalMesh* mesh = loadObject<rt::USkeletalMesh>(path))
        checkText(getPathName(mesh->GetSkeleton()), value.getString(), describe(path, "skeleton"));
}

}

void addMeshChecks(Checks& checks) {
    checks.values.emplace("mesh.bounds", checkBoundsOfStaticMesh);
    checks.values.emplace("mesh.lod", checkLods);
    checks.values.emplace("mesh.lods", checkLodCount);
    checks.values.emplace("mesh.materials", checkMaterials);
    checks.values.emplace("mesh.nanite", checkNanite);
    checks.values.emplace("skmesh.bounds", checkBoundsOfSkeletalMesh);
    checks.values.emplace("skmesh.materials", checkSkeletalMaterials);
    checks.values.emplace("skmesh.skeleton", checkSkeleton);
}

}
