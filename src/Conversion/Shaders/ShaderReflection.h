#pragma once

#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Conversion/Error.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/D3D12RHI/D3D12Shaders.h"

namespace ue5parse::conversion {

class MaterialLayout;

class ShaderReflection {
public:
    struct SignatureElement {
        std::string semantic;
        std::uint32_t semanticIndex = 0;
        std::uint32_t reg = 0;
        std::uint8_t mask = 0;
        bool integer = false;
    };
    struct Member {
        std::string name;
        std::uint32_t offset = 0;
    };
    struct ConstantBuffer {
        std::string name;
        std::uint32_t reg = 0;
        std::vector<Member> members;
    };
    struct Binding {
        std::string name;
        std::uint32_t reg = 0;
    };

    ConversionResult<void> read(const runtime::FD3D12ShaderData& code, EGame game, const MaterialLayout& material);

    const SignatureElement* input(std::uint32_t reg, std::uint32_t component) const;
    const std::vector<SignatureElement>& getInputs() const { return inputs; }
    const std::vector<SignatureElement>& getOutputs() const { return outputs; }
    const std::vector<ConstantBuffer>& getConstantBuffers() const { return constantBuffers; }
    const std::vector<Binding>& getTextures() const { return textures; }
    const ConstantBuffer* constantBuffer(std::uint32_t reg) const;
    std::optional<std::pair<const Member*, std::uint32_t>> memberAt(const ConstantBuffer& buffer, std::uint32_t offset) const;
    const Binding* texture(std::uint32_t reg) const;
    bool hasMember(std::string_view name) const;
    const std::vector<std::string>& getProblems() const { return problems; }

private:
    ConversionResult<void> readSignature(const std::map<std::string, std::span<const std::uint8_t>>& chunks);
    static ConversionResult<void> readSignature(std::span<const std::uint8_t> chunk, bool extended, std::vector<SignatureElement>& elements);
    ConversionResult<void> readRdef(std::span<const std::uint8_t> chunk);
    ConversionResult<void> readDxilReflection(std::span<const std::uint8_t> container);

    std::vector<SignatureElement> inputs;
    std::vector<SignatureElement> outputs;
    std::vector<ConstantBuffer> constantBuffers;
    std::vector<Binding> textures;
    std::vector<std::string> problems;
};

}
