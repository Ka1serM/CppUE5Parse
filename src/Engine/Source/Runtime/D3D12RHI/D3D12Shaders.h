#pragma once

#include <span>

#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/RenderCore/ShaderCore.h"

namespace ue5parse::runtime {

class FD3D12ShaderData {
public:
    Result<void> Read(std::span<const uint8> InCode, EGame Game);

    FShaderResourceTable ShaderResourceTable;
    std::span<const uint8> Code;
    TArray<FString> UniformBufferNames;
};

}
