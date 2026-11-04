#pragma once

#include <vector>

#include "Conversion/Materials/MaterialData.h"
#include "Conversion/Materials/Warnings.h"
#include "Conversion/Spirv/ExpressionGraph.h"

namespace ue5parse::conversion {

std::vector<ExprId> materialUniforms(ExpressionGraph& graph, const MaterialData& data, Warnings& warnings);

}
