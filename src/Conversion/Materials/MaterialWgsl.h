#pragma once

#include <array>
#include <string>
#include <vector>

#include "Conversion/Materials/MaterialProgram.h"

namespace ue5parse::conversion {

//! The WGSL holds evaluateMaterial and evaluateDisplacement for the material fragment module of the WebGPU backend, as MaterialShell writes them in SPIR-V.
struct MaterialWgsl {
    std::string code;
    std::vector<std::array<float, 4>> constants;
};

MaterialWgsl writeMaterialWgsl(const MaterialProgram& program);

}
