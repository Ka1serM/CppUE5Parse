#include "Conversion/Shaders/ShaderReflection.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <map>
#include <memory>
#include <string_view>

#include <dxil_spirv_c.h>

#include "Conversion/Shaders/EngineShaderParameterLayouts.h"
#include "Conversion/Shaders/MaterialLayout.h"
#include "Engine/Source/Runtime/RHI/RHIDefinitions.h"

namespace ue5parse::conversion {
namespace {

class ChunkReader {
public:
    explicit ChunkReader(const std::span<const std::uint8_t> bytes) : bytes(bytes) {}

    bool isTruncated() const { return truncated; }

    std::uint32_t word(const std::size_t offset) {
        if (offset + 4 > bytes.size()) {
            truncated = true;
            return 0;
        }
        std::uint32_t value;
        std::memcpy(&value, bytes.data() + offset, 4);
        return value;
    }

    std::uint8_t byte(const std::size_t offset) {
        if (offset >= bytes.size()) {
            truncated = true;
            return 0;
        }
        return bytes[offset];
    }

    std::string string(const std::size_t offset) {
        if (offset >= bytes.size()) {
            truncated = true;
            return {};
        }
        const auto* begin = reinterpret_cast<const char*>(bytes.data() + offset);
        return {begin, strnlen(begin, bytes.size() - offset)};
    }

private:
    std::span<const std::uint8_t> bytes;
    bool truncated = false;
};

ConversionResult<std::map<std::string, std::span<const std::uint8_t>>> chunks(const std::span<const std::uint8_t> container) {
    if (container.size() < 32 || std::memcmp(container.data(), "DXBC", 4) != 0)
        return conversionFailure("The shader holds no DXBC container.");
    ChunkReader reader(container);
    std::map<std::string, std::span<const std::uint8_t>> result;
    const std::uint32_t count = reader.word(28);
    for (std::uint32_t i = 0; i < count && !reader.isTruncated(); ++i) {
        const std::uint32_t offset = reader.word(32 + static_cast<std::size_t>(i) * 4);
        const std::uint32_t size = reader.word(static_cast<std::size_t>(offset) + 4);
        if (static_cast<std::size_t>(offset) + 8 + size > container.size())
            return conversionFailure("A chunk of the shader container is truncated.");
        result.emplace(std::string(reinterpret_cast<const char*>(container.data() + offset), 4), container.subspan(static_cast<std::size_t>(offset) + 8, size));
    }
    if (reader.isTruncated())
        return conversionFailure("The shader container is truncated.");
    return result;
}

std::optional<std::uint32_t> trailingNumber(const std::string_view text) {
    const std::size_t space = text.rfind(' ');
    const std::string_view digits = space == std::string_view::npos ? text : text.substr(space + 1);
    std::uint32_t value = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (error != std::errc() || end != digits.data() + digits.size())
        return std::nullopt;
    return value;
}

//! dxil-spirv prints the reflection module as LLVM IR text.
class Metadata {
public:
    explicit Metadata(const std::string_view text) {
        std::size_t lineStart = 0;
        while (lineStart < text.size()) {
            std::size_t lineEnd = text.find('\n', lineStart);
            if (lineEnd == std::string_view::npos)
                lineEnd = text.size();
            const std::string_view line = text.substr(lineStart, lineEnd - lineStart);
            lineStart = lineEnd + 1;
            if (line.empty() || line[0] != '!')
                continue;
            const std::size_t equals = line.find(" = !{");
            if (equals == std::string_view::npos || line.back() != '}')
                continue;
            nodes[std::string(line.substr(1, equals - 1))] = split(line.substr(equals + 5, line.size() - equals - 6));
        }
    }

    const std::vector<std::string>& node(const std::string& reference) const {
        static const std::vector<std::string> empty;
        if (reference.empty() || reference[0] != '!')
            return empty;
        const auto found = nodes.find(reference.substr(1));
        return found != nodes.end() ? found->second : empty;
    }

    static std::string text(const std::string& item) { return item.size() >= 2 && item.front() == '"' ? item.substr(1, item.size() - 2) : std::string(); }

    static std::string type(const std::string& item) {
        std::string result = item.substr(0, item.rfind(' '));
        if (!result.empty() && result.back() == '*')
            result.pop_back();
        return result;
    }

private:
    static std::vector<std::string> split(const std::string_view body) {
        std::vector<std::string> items;
        int depth = 0;
        bool quoted = false;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= body.size(); ++i) {
            const char character = i < body.size() ? body[i] : ',';
            if (character == '"')
                quoted = !quoted;
            if (quoted)
                continue;
            if (character == '{' || character == '[' || character == '<')
                ++depth;
            if (character == '}' || character == ']' || character == '>')
                --depth;
            if (character != ',' || depth != 0)
                continue;
            std::string_view item = body.substr(start, i - start);
            while (!item.empty() && item.front() == ' ')
                item.remove_prefix(1);
            items.emplace_back(item);
            start = i + 1;
        }
        return items;
    }

    std::map<std::string, std::vector<std::string>> nodes;
};

}

ConversionResult<void> ShaderReflection::read(const runtime::FD3D12ShaderData& code, const EGame game, const MaterialLayout& material) {
    const auto parts = chunks(code.Code);
    if (!parts)
        return std::unexpected(parts.error());
    if (auto signature = readSignature(*parts); !signature)
        return signature;

    const auto rdef = parts->find("RDEF");
    if (rdef != parts->end() || parts->contains("STAT")) {
        auto containerReflection = rdef != parts->end() ? readRdef(rdef->second) : readDxilReflection(code.Code);
        if (!containerReflection)
            return containerReflection;
        for (ConstantBuffer& buffer : constantBuffers)
            std::ranges::stable_sort(buffer.members, {}, &Member::offset);
        return {};
    }

    const EngineShaderParameterLayouts& layouts = getEngineShaderParameterLayouts(game);
    const auto describe = [&](const std::string& what) { return what + " (the game may need its own layout entry)"; };

    std::vector<std::vector<std::string>> resources(static_cast<std::size_t>(code.UniformBufferNames.Num()));
    for (std::uint32_t slot = 0; slot < resources.size(); ++slot) {
        const std::string name = code.UniformBufferNames[static_cast<runtime::int32>(slot)].ToUtf8();
        ConstantBuffer& buffer = constantBuffers.emplace_back(ConstantBuffer{.name = name.empty() ? "$Globals" : name, .reg = slot, .members = {}});
        const auto prefixed = [&](const std::string_view member) { return name + "_" + std::string(member); };
        if (name.empty())
            continue;
        if (name == "Material") {
            for (const MaterialLayout::Constant& constant : material.getConstants())
                buffer.members.push_back({prefixed(constant.name), constant.offset});
            for (const std::string& resource : material.getResources())
                resources[slot].push_back(prefixed(resource));
        } else if (const auto engine = std::ranges::find(layouts.buffers, name, &EngineUniformBuffer::name); engine != layouts.buffers.end()) {
            for (const ShaderParameterLayoutMember& member : engine->layout.members)
                buffer.members.push_back({prefixed(member.name), member.offset});
            for (const std::string_view resource : engine->layout.resources)
                resources[slot].push_back(prefixed(resource));
        } else {
            problems.push_back(describe("uniform buffer " + name + " in slot b" + std::to_string(slot) + " has no layout"));
        }
        std::ranges::stable_sort(buffer.members, {}, &Member::offset);
    }

    for (const runtime::TArray<std::uint32_t>* map : {&code.ShaderResourceTable.TextureMap, &code.ShaderResourceTable.ShaderResourceViewMap}) {
        const auto entries = runtime::GetResourceTableEntries({map->GetData(), static_cast<std::size_t>(map->Num())});
        if (!entries)
            return conversionFailure(entries.error());
        for (const std::uint32_t entry : *entries) {
            const std::uint32_t uniformBuffer = runtime::FRHIResourceTableEntry::GetUniformBufferIndex(entry);
            const std::uint32_t resource = runtime::FRHIResourceTableEntry::GetResourceIndex(entry);
            const std::uint32_t reg = runtime::FRHIResourceTableEntry::GetBindIndex(entry);
            if (uniformBuffer < resources.size() && resource < resources[uniformBuffer].size())
                textures.push_back({resources[uniformBuffer][resource], reg});
            else
                problems.push_back(describe("resource " + std::to_string(resource) + " of uniform buffer slot b" + std::to_string(uniformBuffer) + " bound to t"
                                            + std::to_string(reg) + " has no name"));
        }
    }
    return {};
}

ConversionResult<void> ShaderReflection::readSignature(const std::map<std::string, std::span<const std::uint8_t>>& parts) {
    if (const auto found = parts.find("OSG1"); found != parts.end()) {
        if (auto read = readSignature(found->second, true, outputs); !read)
            return read;
    } else if (const auto found = parts.find("OSGN"); found != parts.end()) {
        if (auto read = readSignature(found->second, false, outputs); !read)
            return read;
    }
    if (const auto found = parts.find("ISG1"); found != parts.end())
        return readSignature(found->second, true, inputs);
    if (const auto found = parts.find("ISGN"); found != parts.end())
        return readSignature(found->second, false, inputs);
    return conversionFailure("The shader has no input signature.");
}

ConversionResult<void> ShaderReflection::readSignature(const std::span<const std::uint8_t> chunk, const bool extended,
                                                       std::vector<SignatureElement>& elements) {
    constexpr std::uint32_t float32 = 3;
    ChunkReader reader(chunk);
    const std::uint32_t count = reader.word(0);
    const std::uint32_t first = reader.word(4);
    const std::uint32_t stride = extended ? 32 : 24;
    const std::uint32_t base = extended ? 4 : 0;
    for (std::uint32_t i = 0; i < count && !reader.isTruncated(); ++i) {
        const std::size_t element = first + static_cast<std::size_t>(i) * stride + base;
        SignatureElement input{
            .semantic = reader.string(reader.word(element)),
            .semanticIndex = reader.word(element + 4),
            .reg = reader.word(element + 16),
            .mask = reader.byte(element + 20),
            .integer = reader.word(element + 12) != float32,
        };
        if (input.semantic.ends_with("_centroid")) {
            input.semantic.resize(input.semantic.size() - std::string_view("_centroid").size());
            const std::size_t digits = input.semantic.find_last_not_of("0123456789") + 1;
            if (digits < input.semantic.size()) {
                const std::optional<std::uint32_t> index = trailingNumber(std::string_view(input.semantic).substr(digits));
                if (!index)
                    return conversionFailure("The input signature of the shader is malformed.");
                input.semanticIndex = *index;
                input.semantic.resize(digits);
            }
        }
        elements.push_back(std::move(input));
    }
    if (reader.isTruncated())
        return conversionFailure("The input signature of the shader is truncated.");
    return {};
}

ConversionResult<void> ShaderReflection::readRdef(const std::span<const std::uint8_t> chunk) {
    constexpr std::uint32_t inputCBuffer = 0, inputTexture = 2;
    ChunkReader reader(chunk);
    const std::uint32_t bufferCount = reader.word(0), bufferOffset = reader.word(4);
    const std::uint32_t bindingCount = reader.word(8), bindingOffset = reader.word(12);
    const bool shaderModel5 = reader.byte(25) >= 5;

    std::map<std::string, std::uint32_t> bufferRegisters;
    for (std::uint32_t i = 0; i < bindingCount && !reader.isTruncated(); ++i) {
        const std::size_t binding = bindingOffset + static_cast<std::size_t>(i) * 32;
        const std::string name = reader.string(reader.word(binding));
        const std::uint32_t type = reader.word(binding + 4);
        const std::uint32_t reg = reader.word(binding + 20);
        if (type == inputCBuffer)
            bufferRegisters[name] = reg;
        if (type == inputTexture)
            textures.push_back({name, reg});
    }
    for (std::uint32_t i = 0; i < bufferCount && !reader.isTruncated(); ++i) {
        const std::size_t buffer = bufferOffset + static_cast<std::size_t>(i) * 24;
        ConstantBuffer result{.name = reader.string(reader.word(buffer)), .reg = 0, .members = {}};
        const auto reg = bufferRegisters.find(result.name);
        if (reg == bufferRegisters.end())
            continue;
        result.reg = reg->second;
        const std::uint32_t variableCount = reader.word(buffer + 4), variableOffset = reader.word(buffer + 8);
        for (std::uint32_t v = 0; v < variableCount && !reader.isTruncated(); ++v) {
            const std::size_t variable = variableOffset + static_cast<std::size_t>(v) * (shaderModel5 ? 40 : 24);
            result.members.push_back({reader.string(reader.word(variable)), reader.word(variable + 4)});
        }
        constantBuffers.push_back(std::move(result));
    }
    if (reader.isTruncated())
        return conversionFailure("The reflection of the shader is truncated.");
    return {};
}

ConversionResult<void> ShaderReflection::readDxilReflection(const std::span<const std::uint8_t> container) {
    dxil_spv_parsed_blob blob = nullptr;
    if (dxil_spv_parse_reflection_dxil_blob(container.data(), container.size(), &blob) != DXIL_SPV_SUCCESS)
        return conversionFailure("dxil-spirv cannot parse the DXIL reflection module.");
    const std::unique_ptr<std::remove_pointer_t<dxil_spv_parsed_blob>, void (*)(dxil_spv_parsed_blob)> owner(blob, dxil_spv_parsed_blob_free);
    const char* text = nullptr;
    if (dxil_spv_parsed_blob_get_disassembled_ir(blob, &text) != DXIL_SPV_SUCCESS || text == nullptr)
        return conversionFailure("dxil-spirv cannot disassemble the DXIL reflection module.");
    const std::string_view ir(text);
    const Metadata metadata(ir);

    const auto named = [&](const std::string_view name) -> std::string {
        const std::size_t at = ir.find(std::string(name) + " = !{!");
        if (at == std::string_view::npos)
            return {};
        const std::size_t start = at + name.size() + 5;
        return std::string(ir.substr(start, ir.find('}', start) - start));
    };
    const std::string resourceList = named("!dx.resources");
    if (resourceList.empty())
        return {};
    const auto& resources = metadata.node(resourceList);
    if (resources.size() != 4)
        return conversionFailure("The DXIL resource metadata of the shader is malformed.");
    bool numbersValid = true;
    const auto integer = [&](const std::string& item) {
        const std::optional<std::uint32_t> value = trailingNumber(item);
        numbersValid = numbersValid && value.has_value();
        return value.value_or(0);
    };

    std::map<std::string, std::string> fieldsByType;
    const std::string annotations = named("!dx.typeAnnotations");
    for (std::size_t start = 0; start < annotations.size();) {
        std::size_t end = annotations.find(", ", start);
        if (end == std::string::npos)
            end = annotations.size();
        const auto& annotation = metadata.node(annotations.substr(start, end - start));
        if (!annotation.empty() && integer(annotation[0]) == 0)
            for (std::size_t i = 1; i + 1 < annotation.size(); i += 2)
                fieldsByType[Metadata::type(annotation[i])] = annotation[i + 1];
        start = end + 2;
    }

    for (const std::string& reference : metadata.node(resources[0])) {
        const auto& resource = metadata.node(reference);
        if (resource.size() < 5)
            return conversionFailure("The DXIL resource metadata of the shader is malformed.");
        textures.push_back({Metadata::text(resource[2]), integer(resource[4])});
    }
    for (const std::string& reference : metadata.node(resources[2])) {
        const auto& resource = metadata.node(reference);
        if (resource.size() < 5)
            return conversionFailure("The DXIL resource metadata of the shader is malformed.");
        ConstantBuffer buffer{.name = Metadata::text(resource[2]), .reg = integer(resource[4]), .members = {}};
        const auto fields = fieldsByType.find(Metadata::type(resource[1]));
        if (fields == fieldsByType.end())
            return conversionFailure("The DXIL reflection has no layout for the constant buffer " + buffer.name + ".");
        const auto& list = metadata.node(fields->second);
        for (std::size_t i = 1; i < list.size(); ++i) {
            const auto& field = metadata.node(list[i]);
            for (std::size_t tag = 2; tag + 1 < field.size(); tag += 2)
                if (integer(field[tag]) == 3)
                    buffer.members.push_back({Metadata::text(field[1]), integer(field[tag + 1])});
        }
        constantBuffers.push_back(std::move(buffer));
    }
    if (!numbersValid)
        return conversionFailure("The DXIL reflection of the shader has a malformed number.");
    return {};
}

const ShaderReflection::SignatureElement* ShaderReflection::input(const std::uint32_t reg, const std::uint32_t component) const {
    for (const SignatureElement& element : inputs)
        if (element.reg == reg && (element.mask & (1u << component)) != 0)
            return &element;
    return nullptr;
}

const ShaderReflection::ConstantBuffer* ShaderReflection::constantBuffer(const std::uint32_t reg) const {
    for (const ConstantBuffer& buffer : constantBuffers)
        if (buffer.reg == reg)
            return &buffer;
    return nullptr;
}

std::optional<std::pair<const ShaderReflection::Member*, std::uint32_t>> ShaderReflection::memberAt(const ConstantBuffer& buffer,
                                                                                                    const std::uint32_t offset) const {
    const auto after = std::ranges::upper_bound(buffer.members, offset, {}, &Member::offset);
    if (after == buffer.members.begin())
        return std::nullopt;
    const Member& found = *std::prev(after);
    return std::pair{&found, offset - found.offset};
}

const ShaderReflection::Binding* ShaderReflection::texture(const std::uint32_t reg) const {
    for (const Binding& binding : textures)
        if (binding.reg == reg)
            return &binding;
    return nullptr;
}

bool ShaderReflection::hasMember(const std::string_view name) const {
    for (const ConstantBuffer& buffer : constantBuffers)
        for (const Member& member : buffer.members)
            if (member.name == name)
                return true;
    return false;
}

}
