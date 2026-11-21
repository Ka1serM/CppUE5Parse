#pragma once

#include "Engine/Source/Runtime/Engine/MaterialShared.h"

namespace ue5parse::runtime {

void SerializeLegacyUniformExpressionSet(FArchive& Ar, FUniformExpressionSet& Value);

}
