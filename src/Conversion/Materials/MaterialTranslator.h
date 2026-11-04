#pragma once

#include <string>
#include <vector>

#include "Conversion/Error.h"
#include "Conversion/Materials/MaterialData.h"
#include "Conversion/Materials/MaterialProgram.h"
#include "Conversion/Materials/MaterialShaderCompiler.h"
#include "Conversion/Materials/Surface.h"

namespace ue5parse::conversion {

struct MaterialTranslation {
    MaterialProgram program;
    std::vector<std::string> warnings;
};

ConversionResult<MaterialTranslation> translateMaterial(const MaterialData& data, const std::vector<CompiledMaterialShader>& shaders,
                                                        const SurfaceOverride& surfaceOverride = {});

}
