#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

namespace test::assets {

namespace {

void checkSize(const std::string& path, const Json& value) {
    rt::UTexture2D* texture = loadObject<rt::UTexture2D>(path);
    if (texture == nullptr)
        return;
    double size[] = {static_cast<double>(texture->GetSizeX()), static_cast<double>(texture->GetSizeY())};
    checkNumbers(size, value, 0.0, describe(path, "size"));
}

void checkSettings(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "CompressionSettings", value.getArray()[0]);
    checkReflectedProperty(path, "SRGB", value.getArray()[1]);
}

}

void addTextureChecks(Checks& checks) {
    checks.values.emplace("texture.settings", checkSettings);
    checks.values.emplace("texture.size", checkSize);
}

}
