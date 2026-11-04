#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"

namespace ue5parse::conversion {

class MaterialLayout {
public:
    struct Constant {
        std::string name;
        std::uint32_t offset;
    };

    MaterialLayout(const runtime::FUniformExpressionSet& expressions, EGame game);

    const std::vector<Constant>& getConstants() const { return constants; }
    const std::vector<std::string>& getResources() const { return resources; }

private:
    std::vector<Constant> constants;
    std::vector<std::string> resources;
};

}
