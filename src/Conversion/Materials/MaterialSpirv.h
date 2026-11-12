#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Conversion/Error.h"
#include "Conversion/Materials/MaterialProgram.h"

namespace ue5parse::conversion {

struct MaterialSpirv {
    std::vector<uint32_t> words;
    std::vector<std::array<float, 4>> constants;
};

class MaterialShell {
public:
    class Module;

    explicit MaterialShell(std::span<const uint32_t> words);
    ~MaterialShell();
    MaterialShell(const MaterialShell&) = delete;
    MaterialShell& operator=(const MaterialShell&) = delete;

    ConversionResult<MaterialSpirv> write(const MaterialProgram& program) const;

private:
    std::unique_ptr<Module> module;
    std::optional<std::string> failure;
};

}
