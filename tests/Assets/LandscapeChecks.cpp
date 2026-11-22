#include <algorithm>
#include <format>
#include <limits>
#include <string>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Level.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Landscape/LandscapeComponent.h"
#include "Engine/Source/Runtime/Landscape/LandscapeDataAccess.h"
#include "Engine/Source/Runtime/Landscape/LandscapeProxy.h"
#include "Engine/Source/Runtime/Landscape/LandscapeTextureStorageProvider.h"

namespace test::assets {

namespace {

const rt::ULandscapeTextureStorageProviderFactory* findStorageProvider(const rt::UTexture2D* texture) {
    if (texture == nullptr)
        return nullptr;
    for (const rt::TObjectPtr<rt::UAssetUserData>& userData : texture->AssetUserData)
        if (const auto* factory = rt::Cast<rt::ULandscapeTextureStorageProviderFactory>(userData.Get()))
            return factory;
    return nullptr;
}

int toTextureCoord(int vertex, const rt::ULandscapeComponent& component) {
    const int subsection = std::min(vertex / component.SubsectionSizeQuads, component.NumSubsections - 1);
    return subsection * (component.SubsectionSizeQuads + 1) + vertex - subsection * component.SubsectionSizeQuads;
}

//! UE sets the cached local box Z to the lowest and highest mip 0 height of the component's vertices.
void checkComponentHeights(const rt::ULandscapeComponent& component, const rt::ULandscapeTextureStorageProviderFactory& factory) {
    const std::string what = getPathName(&component);
    if (!CHECK(factory.Mips.Num() > 0))
        return;
    ue5parse::Result<std::vector<std::byte>> pixels = factory.GetMipData(0);
    if (!pixels)
        printError(what, pixels.error());
    if (!CHECK(pixels.has_value()))
        return;
    const int width = factory.Mips[0].SizeX;
    const int height = factory.Mips[0].SizeY;
    const int offsetX = static_cast<int>(component.HeightmapScaleBias.Z * width);
    const int offsetY = static_cast<int>(component.HeightmapScaleBias.W * height);
    float lowest = std::numeric_limits<float>::max();
    float highest = std::numeric_limits<float>::lowest();
    for (int y = 0; y <= component.ComponentSizeQuads; y++)
        for (int x = 0; x <= component.ComponentSizeQuads; x++) {
            const int texX = offsetX + toTextureCoord(x, component);
            const int texY = offsetY + toTextureCoord(y, component);
            if (!CHECK(texX < width && texY < height))
                return;
            const std::size_t at = (static_cast<std::size_t>(texY) * width + texX) * 4;
            const auto value = static_cast<rt::uint16>(std::to_integer<int>((*pixels)[at + 2]) * 256 + std::to_integer<int>((*pixels)[at + 1]));
            lowest = std::min(lowest, rt::LandscapeDataAccess::GetLocalHeight(value));
            highest = std::max(highest, rt::LandscapeDataAccess::GetLocalHeight(value));
        }
    checkNear(lowest, component.CachedLocalBox.Min.Z, 0.0, describe(what, "CachedLocalBox.Min.Z"));
    checkNear(highest, component.CachedLocalBox.Max.Z, 0.0, describe(what, "CachedLocalBox.Max.Z"));
}

void checkHeightmaps(const Cook& cook) {
    const Json* assets = cook.expected.find("assets");
    if (assets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    withRuntime(cook, [&](const ue5parse::FileProvider&) {
        for (const auto& [path, assetClass] : assets->getObject()) {
            if (assetClass.getString() != "World")
                continue;
            const rt::UWorld* world = loadObject<rt::UWorld>(path);
            if (world == nullptr || !CHECK(world->PersistentLevel != nullptr))
                continue;
            for (const rt::AActor* actor : world->PersistentLevel->Actors)
                if (const auto* proxy = rt::Cast<rt::ALandscapeProxy>(actor))
                    for (const rt::ULandscapeComponent* component : proxy->LandscapeComponents)
                        if (const rt::ULandscapeTextureStorageProviderFactory* factory = component == nullptr ? nullptr : findStorageProvider(component->HeightmapTexture))
                            checkComponentHeights(*component, *factory);
        }
    });
}

}

void addLandscapeChecks(Checks& checks) {
    checks.cook.emplace("landscape.heightmap", checkHeightmaps);
}

}
