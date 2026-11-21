#pragma once

#include "Conversion/Textures/TextureDecoder.h"
#include "Engine/Source/Runtime/Engine/VT/VirtualTextureBuiltData.h"

namespace ue5parse::conversion {

ConversionResult<DecodedImage> decodeVirtualTexture(const runtime::FVirtualTextureBuiltData& data, unsigned layer, unsigned level);

}
