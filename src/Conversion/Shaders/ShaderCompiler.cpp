#include "Conversion/Shaders/ShaderCompiler.h"

#include <cstring>
#include <memory>
#include <span>

#include <dxil_spirv_c.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include "Engine/Source/Runtime/D3D12RHI/D3D12Shaders.h"

namespace ue5parse::conversion {
namespace {

dxil_spv_bool mapSrv(void*, const dxil_spv_d3d_binding* binding, dxil_spv_srv_vulkan_binding* vulkan) {
    *vulkan = {};
    vulkan->buffer_binding.set = binding->register_space;
    vulkan->buffer_binding.binding = binding->register_index;
    return DXIL_SPV_TRUE;
}

dxil_spv_bool mapSampler(void*, const dxil_spv_d3d_binding* binding, dxil_spv_vulkan_binding* vulkan) {
    *vulkan = {};
    vulkan->set = binding->register_space;
    vulkan->binding = binding->register_index;
    return DXIL_SPV_TRUE;
}

dxil_spv_bool mapCbv(void*, const dxil_spv_d3d_binding* binding, dxil_spv_cbv_vulkan_binding* vulkan) {
    *vulkan = {};
    vulkan->vulkan.uniform_binding.set = binding->register_space;
    vulkan->vulkan.uniform_binding.binding = binding->register_index;
    return DXIL_SPV_TRUE;
}

ConversionResult<std::vector<std::uint32_t>> convert(const std::span<const std::uint8_t> container) {
    dxil_spv_parsed_blob blob = nullptr;
    if (dxil_spv_parse_dxil_blob(container.data(), container.size(), &blob) != DXIL_SPV_SUCCESS)
        return conversionFailure("dxil-spirv cannot parse the shader container.");
    const std::unique_ptr<std::remove_pointer_t<dxil_spv_parsed_blob>, void (*)(dxil_spv_parsed_blob)> blobOwner(blob, dxil_spv_parsed_blob_free);
    dxil_spv_converter converter = nullptr;
    if (dxil_spv_create_converter(blob, &converter) != DXIL_SPV_SUCCESS)
        return conversionFailure("dxil-spirv cannot create the SPIR-V converter.");
    const std::unique_ptr<std::remove_pointer_t<dxil_spv_converter>, void (*)(dxil_spv_converter)> converterOwner(converter, dxil_spv_converter_free);
    dxil_spv_converter_set_srv_remapper(converter, mapSrv, nullptr);
    dxil_spv_converter_set_sampler_remapper(converter, mapSampler, nullptr);
    dxil_spv_converter_set_cbv_remapper(converter, mapCbv, nullptr);
    if (dxil_spv_converter_run(converter) != DXIL_SPV_SUCCESS)
        return conversionFailure("dxil-spirv cannot convert the shader to SPIR-V.");
    dxil_spv_compiled_spirv compiled{};
    if (dxil_spv_converter_get_compiled_spirv(converter, &compiled) != DXIL_SPV_SUCCESS)
        return conversionFailure("dxil-spirv produced no SPIR-V module.");
    std::vector<std::uint32_t> words(compiled.size / sizeof(std::uint32_t));
    std::memcpy(words.data(), compiled.data, words.size() * sizeof(std::uint32_t));
    return words;
}

ConversionResult<std::vector<std::uint32_t>> optimize(const std::vector<std::uint32_t>& module) {
    std::string diagnostic;
    const auto collect = [&](const spv_message_level_t level, const char*, const spv_position_t&, const char* message) {
        if (level <= SPV_MSG_ERROR)
            diagnostic += message;
    };
    spvtools::Optimizer optimizer(SPV_ENV_UNIVERSAL_1_6);
    optimizer.SetMessageConsumer(collect);
    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass())
        .RegisterPass(spvtools::CreateMergeReturnPass())
        .RegisterPass(spvtools::CreateInlineExhaustivePass())
        .RegisterPass(spvtools::CreateEliminateDeadFunctionsPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreatePrivateToLocalPass())
        .RegisterPass(spvtools::CreateLocalSingleBlockLoadStoreElimPass())
        .RegisterPass(spvtools::CreateLocalSingleStoreElimPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateScalarReplacementPass(0))
        .RegisterPass(spvtools::CreateLocalAccessChainConvertPass())
        .RegisterPass(spvtools::CreateLocalSingleBlockLoadStoreElimPass())
        .RegisterPass(spvtools::CreateLocalSingleStoreElimPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateLocalMultiStoreElimPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateCCPPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateLoopUnrollPass(true))
        .RegisterPass(spvtools::CreateDeadBranchElimPass())
        .RegisterPass(spvtools::CreateRedundancyEliminationPass())
        .RegisterPass(spvtools::CreateCombineAccessChainsPass())
        .RegisterPass(spvtools::CreateSimplificationPass())
        .RegisterPass(spvtools::CreateScalarReplacementPass(0))
        .RegisterPass(spvtools::CreateLocalAccessChainConvertPass())
        .RegisterPass(spvtools::CreateLocalSingleBlockLoadStoreElimPass())
        .RegisterPass(spvtools::CreateLocalSingleStoreElimPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateSSARewritePass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateVectorDCEPass())
        .RegisterPass(spvtools::CreateDeadInsertElimPass())
        .RegisterPass(spvtools::CreateDeadBranchElimPass())
        .RegisterPass(spvtools::CreateSimplificationPass())
        .RegisterPass(spvtools::CreateIfConversionPass())
        .RegisterPass(spvtools::CreateCopyPropagateArraysPass())
        .RegisterPass(spvtools::CreateReduceLoadSizePass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass())
        .RegisterPass(spvtools::CreateBlockMergePass())
        .RegisterPass(spvtools::CreateRedundancyEliminationPass())
        .RegisterPass(spvtools::CreateDeadBranchElimPass())
        .RegisterPass(spvtools::CreateBlockMergePass())
        .RegisterPass(spvtools::CreateSimplificationPass());
    std::vector<std::uint32_t> optimized;
    if (!optimizer.Run(module.data(), module.size(), &optimized))
        return conversionFailure("SPIRV-Tools cannot optimize the shader: " + diagnostic);
    spvtools::SpirvTools validator(SPV_ENV_UNIVERSAL_1_6);
    validator.SetMessageConsumer(collect);
    if (!validator.Validate(optimized))
        return conversionFailure("SPIRV-Tools rejects the optimized shader: " + diagnostic);
    return optimized;
}

}

ConversionResult<CompiledShader> compileShader(const std::span<const std::uint8_t> code, const EGame game, const MaterialLayout& material) {
    runtime::FD3D12ShaderData shader;
    if (auto read = shader.Read(code, game); !read)
        return conversionFailure(read.error());
    CompiledShader compiled;
    if (auto reflection = compiled.reflection.read(shader, game, material); !reflection)
        return std::unexpected(std::move(reflection.error()));
    auto spirv = convert(shader.Code).and_then(optimize);
    if (!spirv)
        return std::unexpected(std::move(spirv.error()));
    compiled.spirv = std::move(*spirv);
    return compiled;
}

}
