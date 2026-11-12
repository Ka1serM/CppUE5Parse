#include "Conversion/Materials/MaterialEvaluator.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <optional>
#include <span>

#include "Conversion/Materials/Preshader.h"

namespace ue5parse::conversion {
namespace {

class UnrealShaderInputs final : public ShaderInputs {
public:
    UnrealShaderInputs(ExpressionGraph& graph, const AbsentValues absentValues, bool& absentValueRead, const CompiledMaterialShader& compiled, const std::vector<ExprId>& material,
                       const TextureSlots& textureSlots, const EngineValues& engine, const std::span<const ParameterCollection> collections,
                       std::vector<NumericParameter>& collectionParameters, const EngineSymbols& symbols, const Interpolants& interpolants,
                       const VertexFactory factory, const uint32_t particleBase, const VertexProjection projection)
        : graph(graph), absentValues(absentValues), absentValueRead(absentValueRead), shader(compiled.type), reflection(compiled.reflection), material(material), textureSlots(textureSlots),
          engine(engine), collections(collections), collectionParameters(collectionParameters), symbols(symbols), interpolants(interpolants),
          vertex(compiled.stage == ShaderStage::Vertex), skinned(vertex && isSkinned(compiled.reflection)), factory(factory), particleBase(particleBase),
          projection(projection), collectionBase(particleBase + (factory == VertexFactory::Mesh ? 0 : static_cast<uint32_t>(particleParameters.size()))) {}

    ExprId constantBuffer(const uint32_t reg, const uint32_t offset) override {
        const ShaderReflection::ConstantBuffer* buffer = reflection.constantBuffer(reg);
        if (buffer == nullptr)
            return failed("The shader has no reflection for constant buffer b" + std::to_string(reg) + ".");
        const auto member = reflection.memberAt(*buffer, offset);
        if (!member)
            return unknown("constant buffer " + buffer->name + " has no member at offset " + std::to_string(offset));
        const std::string& name = member->first->name;
        const uint32_t word = member->second / 4;
        if (const std::string symbol = name + "[" + std::to_string(word) + "]"; symbols.contains(symbol))
            return graph.unresolved(symbol);
        if (name == "Material_PreshaderBuffer")
            return materialWord(name, word);
        if (name == "View_GameTime" || name == "View_RealTime" || name == "View_PrevFrameGameTime" || name == "View_PrevFrameRealTime")
            return graph.time();
        if (name == "View_FrameNumber")
            return graph.frame();
        if (name == "View_DeltaTime")
            return graph.constant(1.0f / 60.0f);
        if (name == "View_Random")
            return graph.unary(ExprOp::Floor, graph.mul(graph.unary(ExprOp::Random, graph.frame()), graph.constant(32768.0f)));
        if (name == "Primitive_CustomPrimitiveData" && word < 36)
            return graph.geometry(static_cast<GeometryAttribute>(static_cast<uint32_t>(GeometryAttribute::CustomData0) + word / 4), word % 4);
        if (factory == VertexFactory::ParticleSprite && name == "SpriteVF_SubImageSize" && word < 4)
            return particle(particleSubImageSize, word);
        if (name == "Primitive_PrimitiveComponentId")
            return graph.geometry(GeometryAttribute::PrimitiveId, 0);
        if (name == "View_ViewTilePosition")
            return graph.constant(0.0f);
        if ((name == "View_RelativePreViewTranslation" || name == "View_PreViewTranslation") && word < 3)
            return graph.negate(translatedOrigin(word));
        if ((name == "Primitive_LocalToWorld" || name == "Primitive_LocalToRelativeWorld") && word < 16)
            return localToWorld(word / 4)[word % 4];
        if ((name == "Primitive_WorldToLocal" || name == "Primitive_RelativeWorldToLocal") && word < 16)
            return worldToLocal(word / 4)[word % 4];
        if (name == "Primitive_TilePosition")
            return graph.constant(0.0f);
        if (vertex && (name == "View_TranslatedWorldToClip" || name == "View_TranslatedWorldToClipArray")) {
            if (projection == VertexProjection::Identity)
                return projectionLeaf(word % 16 / 4, word % 4);
            return objectToClip(word % 16 / 4, word % 4);
        }
        if (name == "View_TranslatedWorldToClip" && word < 16)
            return translatedWorldToClip(word / 4, word % 4);
        if (name == "View_TranslatedWorldToClipArray")
            return translatedWorldToClip(word % 16 / 4, word % 4);
        if ((name == "Primitive_ActorWorldPosition" || name == "Primitive_ActorRelativeWorldPosition") && word < 3)
            return localToWorld(3)[word];
        if (name == "Primitive_ObjectOrientation" && word < 3)
            return objectOrientation(word);
        if (const auto field = primitiveField(name, word))
            return *field;
        if ((name == "View_WorldCameraOrigin" || name == "View_RelativeWorldCameraOrigin") && word < 3)
            return cameraOrigin(word);
        if (name == "View_TranslatedWorldCameraOrigin" && word < 3)
            return translatedCameraOrigin(word);
        if (name == "View_ViewForward" && word < 3)
            return cameraForward(word);
        if ((name == "View_ViewToTranslatedWorld" || name == "View_CameraViewToTranslatedWorld") && word < 16)
            return viewToWorld(word / 4)[word % 4];
        if ((name == "View_TranslatedWorldToView" || name == "View_TranslatedWorldToCameraView") && word < 16)
            return worldToView(word / 4)[word % 4];
        if (name == "View_ViewToClip" && word < 16)
            return viewToClip(word / 4, word % 4);
        if (name == "View_ClipToView" && word < 16)
            return clipToView(word / 4, word % 4);
        if (name == "View_RelativeWorldToClip" && word < 16)
            return worldToClip(word / 4, word % 4);
        if (name == "View_NearPlane")
            return graph.constant(nearPlane);
        if (name == "View_InvDeviceZToWorldZTransform" && word < 4)
            return graph.constant(word == 2 ? 1.0f / nearPlane : 0.0f);
        if (name == "View_SVPositionToTranslatedWorld" && word < 16)
            return svPositionToWorld(word / 4, word % 4);
        if ((name == "View_ViewSizeAndInvSize" || name == "View_BufferSizeAndInvSize") && word < 4)
            return word < 2 ? viewSize(word) : graph.div(graph.constant(1.0f), viewSize(word - 2));
        if (name == "View_ScreenPositionScaleBias" && word < 4)
            return graph.constant(word == 1 ? -0.5f : 0.5f);
        if (const auto value = collectionWord(name, word))
            return *value;
        if (const auto value = engine.find(name); value != engine.end() && word < value->second.size())
            return graph.constant(value->second[word]);
        return unknown("engine value " + name + "[" + std::to_string(word) + "] is not provided");
    }

    ExprId input(const uint32_t location, const uint32_t component) override {
        const ShaderReflection::SignatureElement* element = reflection.input(location, component);
        if (element == nullptr)
            return failed("The shader has no signature element for input " + std::to_string(location) + ".");
        const uint32_t first = static_cast<uint32_t>(std::countr_zero(element->mask));
        const uint32_t c = component - first;
        const std::string& semantic = element->semantic;
        const uint32_t index = element->semanticIndex;
        if (vertex && factory == VertexFactory::ParticleSprite)
            return spriteAttribute(semantic, index, c);
        if (vertex && factory == VertexFactory::ParticleMesh)
            return meshParticleAttribute(semantic, index, c);
        if (vertex)
            return attribute(semantic, index, c);
        if (semantic == "TEXCOORD" && index == 10 && c < 3)
            return direction(GeometryAttribute::Tangent, c);
        if (semantic == "TEXCOORD" && index == 11 && c < 3)
            return direction(GeometryAttribute::Normal, c);
        if (semantic == "TEXCOORD" && index == 11)
            return tangentSign();
        if (semantic == "COLOR" && index == 0)
            return graph.geometry(GeometryAttribute::VertexColor, c);
        if (semantic == "PRIMITIVE_ID")
            return graph.geometry(GeometryAttribute::PrimitiveId, 0);
        if (semantic == "TEXCOORD" && index == 7)
            return graph.constant(c == 3 ? 1.0f : 0.0f);
        if (semantic == "TEXCOORD" && (index == 8 || index == 9) && c < 3)
            return graph.sub(position(c), translatedOrigin(c));
        if (const auto interpolant = interpolants.find({semantic, index}); interpolant != interpolants.end() && c < interpolant->second.size())
            return interpolant->second[c];
        return unknown("input " + semantic + std::to_string(index) + " is not provided");
    }

    ExprId frontFacing() override {
        ExprId facing = graph.constant(0.0f);
        for (uint32_t c = 0; c < 3; ++c)
            facing = graph.add(facing, graph.mul(graph.geometry(GeometryAttribute::Normal, c), graph.geometry(GeometryAttribute::ViewDirection, c)));
        return graph.less(facing, graph.constant(0.0f));
    }
    ExprId fragCoord(const uint32_t component) override { return svPosition(component); }

    std::array<ExprId, 4> sample(const uint32_t reg, const std::span<const ExprId> coordinate) override {
        const ShaderReflection::Binding* binding = reflection.texture(reg);
        const std::string name = binding != nullptr ? binding->name : "t" + std::to_string(reg);
        std::array<ExprId, 4> result{};
        if (name.starts_with("Material_TextureCube_")) {
            result.fill(graph.constant(0.0f));
            return result;
        }
        if (name.starts_with(materialTexture2D)) {
            const BoundTexture* slot = boundTexture(name);
            if (slot == nullptr || coordinate.size() < 2) {
                graph.fail("No texture is bound to " + name + ".");
                return result;
            }
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.texture(slot->program, c, coordinate[0], coordinate[1]);
            return result;
        }
        if (name.ends_with("SceneDepthTexture")) {
            const ExprId depth = graph.geometry(GeometryAttribute::SceneDepth, 0);
            return {graph.div(graph.constant(1.0f), depth), graph.constant(0.0f), graph.constant(0.0f), graph.constant(1.0f)};
        }
        if (name.ends_with("SceneColorCopyTexture")) {
            const float level = engine.at("SceneColor").front();
            return {graph.constant(level), graph.constant(level), graph.constant(level), graph.constant(1.0f)};
        }
        if (symbols.contains(name)) {
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.unresolved(name + "[" + std::to_string(c) + "]");
            return result;
        }
        if (const auto value = engine.find(name); value != engine.end() && value->second.size() == 4) {
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.constant(value->second[c]);
            return result;
        }
        result.fill(unknown("texture " + name + " is not provided"));
        return result;
    }

    std::array<ExprId, 4> fetch(const uint32_t reg, const ExprId index) override {
        const ShaderReflection::Binding* binding = reflection.texture(reg);
        const std::string name = binding != nullptr ? binding->name : "t" + std::to_string(reg);
        if (skinned && name == "BoneMatrices" && graph.isConstant(index)) {
            const uint32_t row = static_cast<uint32_t>(graph.node(index).value) % 3;
            std::array<ExprId, 4> result{};
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.constant(c == row ? 1.0f : 0.0f);
            return result;
        }
        if (vertex && name.starts_with("InstanceVF_VertexFetch_") && graph.isConstant(index))
            return instanceElement(name, static_cast<uint32_t>(graph.node(index).value));
        if (vertex && name.starts_with("LocalVF_VertexFetch_") && graph.isConstant(index))
            if (const auto element = vertexElement(name, static_cast<uint32_t>(graph.node(index).value)))
                return *element;
        //! dxil-spirv reads the float4 structured buffer one 32-bit word per fetch.
        if (name == "View_PrimitiveSceneData")
            if (const auto word = ownRecordWord(index))
                if (const auto value = primitiveData(*word / 4))
                    return {(*value)[*word % 4], graph.constant(0.0f), graph.constant(0.0f), graph.constant(0.0f)};
        std::array<ExprId, 4> result{};
        if (const auto value = engine.find(name); value != engine.end() && value->second.size() == 4) {
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.constant(value->second[c]);
            return result;
        }
        result.fill(unknown("buffer fetch " + name + " is not provided"));
        return result;
    }

    std::array<ExprId, 3> textureSize(const uint32_t reg, const ExprId lod) override {
        const ShaderReflection::Binding* binding = reflection.texture(reg);
        const std::string name = binding != nullptr ? binding->name : "t" + std::to_string(reg);
        std::array<ExprId, 3> result{};
        const BoundTexture* slot = name.starts_with(materialTexture2D) ? boundTexture(name) : nullptr;
        if (slot == nullptr || slot->size[0] == 0) {
            result.fill(unknown("the size of texture " + name + " is not provided"));
            return result;
        }
        const ExprId scale = graph.unary(ExprOp::Exp2, lod);
        for (uint32_t c = 0; c < 3; ++c)
            result[c] = graph.binary(ExprOp::Max, graph.unary(ExprOp::Floor, graph.div(graph.constant(static_cast<float>(slot->size[c])), scale)),
                                     graph.constant(1.0f));
        return result;
    }

    //! Only the clip position's final product with the projection is the identity; a screen position the material reads keeps the camera's.
    void resolveProjection(ShaderOutputs& outputs) {
        if (!vertex || projection != VertexProjection::Identity)
            return;
        std::unordered_map<ExprId, ExprId> camera;
        for (uint32_t r = 0; r < 4; ++r)
            for (uint32_t c = 0; c < 4; ++c)
                camera.emplace(projectionLeaf(r, c), objectToClip(r, c));
        const auto inner = [&](const ExprId id) { return graph.substitute(id, camera); };
        const auto outerLeaf = [&](const ExprId id, const uint32_t c) -> std::optional<bool> {
            for (uint32_t k = 0; k < 4; ++k)
                if (id == projectionLeaf(k, c) || id == projectionLeaf(c, k))
                    return k == c;
            return std::nullopt;
        };
        const auto outer = [&](const auto& self, const ExprId id, const uint32_t c) -> ExprId {
            if (const std::optional<bool> diagonal = outerLeaf(id, c))
                return graph.constant(*diagonal ? 1.0f : 0.0f);
            const ExprNode node = graph.node(id);
            if (node.op == ExprOp::Add || node.op == ExprOp::Sub)
                return graph.binary(node.op, self(self, node.a, c), self(self, node.b, c));
            if (node.op == ExprOp::Mul)
                for (const auto& [leaf, other] : {std::pair{node.a, node.b}, std::pair{node.b, node.a}})
                    if (const std::optional<bool> diagonal = outerLeaf(leaf, c))
                        return *diagonal ? inner(other) : graph.constant(0.0f);
            return inner(id);
        };
        for (uint32_t c = 0; c < 4; ++c)
            outputs.position[c] = outer(outer, outputs.position[c], c);
        for (std::array<ExprId, 4>& target : outputs.targets)
            for (ExprId& value : target)
                value = inner(value);
    }

private:
    ExprId projectionLeaf(const uint32_t r, const uint32_t c) {
        return graph.unresolved("View_TranslatedWorldToClip projection " + std::to_string(r) + std::to_string(c));
    }

    static constexpr std::string_view materialTexture2D = "Material_Texture2D_";

    const BoundTexture* boundTexture(const std::string_view name) const {
        uint32_t index = 0;
        const std::string_view digits = name.substr(materialTexture2D.size());
        if (std::from_chars(digits.data(), digits.data() + digits.size(), index).ec != std::errc())
            return nullptr;
        const auto slot = textureSlots.find({runtime::EMaterialTextureParameterType::Standard2D, index});
        return slot != textureSlots.end() ? &slot->second : nullptr;
    }

    static bool isSkinned(const ShaderReflection& reflection) {
        return std::ranges::any_of(reflection.getInputs(), [](const ShaderReflection::SignatureElement& element) {
            return element.semantic == "ATTRIBUTE" && element.semanticIndex == 3 && element.integer;
        });
    }

    ExprId attribute(const std::string& semantic, const uint32_t index, const uint32_t c) {
        if (semantic != "ATTRIBUTE")
            return unknown("vertex input " + semantic + std::to_string(index) + " is not provided");
        const uint32_t firstTexCoord = skinned ? 5 : 4;
        const uint32_t color = skinned ? 13 : 3;
        if (index == 0)
            return c < 3 ? graph.geometry(GeometryAttribute::LocalPosition, c) : graph.constant(1.0f);
        if (index == 1)
            return c < 3 ? direction(GeometryAttribute::Tangent, c) : graph.constant(0.0f);
        if (index == 2)
            return c < 3 ? direction(GeometryAttribute::Normal, c) : tangentSign();
        if (index == color)
            return graph.geometry(GeometryAttribute::VertexColor, c);
        if (skinned && (index == 3 || index == 14 || index == 15))
            return graph.constant(0.0f);
        if (skinned && index == 4)
            return graph.constant(c == 0 ? 1.0f : 0.0f);
        if (index >= firstTexCoord && index < firstTexCoord + 4) {
            const uint32_t channel = (index - firstTexCoord) * 2 + c / 2;
            return graph.geometry(static_cast<GeometryAttribute>(static_cast<uint32_t>(GeometryAttribute::TexCoord0) + channel), c % 2);
        }
        return unknown("vertex attribute " + std::to_string(index) + " is not provided");
    }

    ExprId spriteAttribute(const std::string& semantic, const uint32_t index, const uint32_t c) {
        if (semantic != "ATTRIBUTE")
            return unknown("vertex input " + semantic + std::to_string(index) + " is not provided");
        switch (index) {
        case 0:
            return c < 3 ? graph.geometry(GeometryAttribute::LocalPosition, c) : particle(particleRelativeTime, 0);
        case 1:
            return c < 3 ? graph.geometry(GeometryAttribute::LocalPosition, c) : unknown("the particle id is not provided");
        case 2:
            if (c < 2)
                return axisLength(c == 0 ? GeometryAttribute::ObjectAxisY : GeometryAttribute::ObjectAxisZ);
            return c == 2 ? graph.constant(0.0f) : particle(particleSubImageIndex, 0);
        case 3:
            return particle(particleColor, c);
        case 4:
            return c < 2 ? graph.geometry(GeometryAttribute::TexCoord0, c) : graph.constant(0.0f);
        case 5:
            return particle(dynamicParameter, c);
        default:
            return unknown("sprite vertex attribute " + std::to_string(index) + " is not provided");
        }
    }

    ExprId meshParticleAttribute(const std::string& semantic, const uint32_t index, const uint32_t c) {
        if (semantic != "ATTRIBUTE" || index <= 3)
            return attribute(semantic, index, c);
        if (index <= 7) {
            const auto set = static_cast<GeometryAttribute>(static_cast<uint32_t>(GeometryAttribute::TexCoord0) + index - 4);
            return c < 2 ? graph.geometry(set, c) : graph.constant(0.0f);
        }
        if (index <= 10)
            return graph.constant(c == index - 8 ? 1.0f : 0.0f);
        if (index == 12 && c == 0)
            return graph.unary(ExprOp::Fract, particle(particleSubImageIndex, 0));
        if (index == 12 && c == 1)
            return particle(particleRelativeTime, 0);
        if (index == 13)
            return particle(dynamicParameter, c);
        if (index == 14)
            return particle(particleColor, c);
        return unknown("mesh particle vertex attribute " + std::to_string(index) + " is not provided");
    }

    ExprId particle(const std::string_view name, const uint32_t c) {
        const auto at = std::ranges::find(particleParameters, name, &NumericParameter::name);
        return graph.parameter(particleBase + static_cast<uint32_t>(at - particleParameters.begin()), c);
    }

    ExprId axisLength(const GeometryAttribute axis) {
        ExprId squared = graph.constant(0.0f);
        for (uint32_t c = 0; c < 3; ++c) {
            const ExprId component = graph.geometry(axis, c);
            squared = graph.add(squared, graph.mul(component, component));
        }
        return graph.unary(ExprOp::Sqrt, squared);
    }

    std::array<ExprId, 4> instanceElement(const std::string& name, const uint32_t element) {
        std::array<ExprId, 4> result{};
        const uint32_t row = name.ends_with("InstanceTransformBuffer") ? element % 3 : 3;
        for (uint32_t c = 0; c < 4; ++c)
            result[c] = graph.constant(c == row ? 1.0f : 0.0f);
        return result;
    }

    std::optional<std::array<ExprId, 4>> vertexElement(const std::string& name, const uint32_t element) {
        std::array<ExprId, 4> result{};
        if (name == "LocalVF_VertexFetch_ColorComponentsBuffer") {
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = graph.geometry(GeometryAttribute::VertexColor, c < 3 ? 2 - c : c);
            return result;
        }
        if (name == "LocalVF_VertexFetch_PackedTangentsBuffer" && element < 2) {
            for (uint32_t c = 0; c < 4; ++c)
                result[c] = attribute("ATTRIBUTE", element + 1, c);
            return result;
        }
        if (name == "LocalVF_VertexFetch_TexCoordBuffer" && element < 8) {
            const auto set = static_cast<GeometryAttribute>(static_cast<uint32_t>(GeometryAttribute::TexCoord0) + element);
            return std::array{graph.geometry(set, 0), graph.geometry(set, 1), graph.constant(0.0f), graph.constant(0.0f)};
        }
        return std::nullopt;
    }

    std::optional<ExprId> collectionWord(const std::string& name, const uint32_t word) {
        constexpr std::string_view prefix = "MaterialCollection", suffix = "_Vectors";
        if (!name.starts_with(prefix) || !name.ends_with(suffix))
            return std::nullopt;
        uint32_t index = 0;
        const char* digits = name.data() + prefix.size();
        const char* end = name.data() + name.size() - suffix.size();
        const auto parsed = std::from_chars(digits, end, index);
        if (parsed.ec != std::errc() || parsed.ptr != end || index >= collections.size())
            return std::nullopt;
        const ParameterCollection& collection = collections[index];
        const auto scalarWords = static_cast<uint32_t>((collection.scalars.size() + 3) / 4 * 4);
        if (word < collection.scalars.size())
            return collectionParameter(collection.scalars[word], 0);
        if (word < scalarWords)
            return graph.constant(0.0f);
        if (const uint32_t vector = (word - scalarWords) / 4; vector < collection.vectors.size())
            return collectionParameter(collection.vectors[vector], (word - scalarWords) % 4);
        return std::nullopt;
    }

    ExprId collectionParameter(const NumericParameter& parameter, const uint32_t c) {
        auto at = std::ranges::find(collectionParameters, parameter.name, &NumericParameter::name);
        if (at == collectionParameters.end())
            at = collectionParameters.insert(at, parameter);
        return graph.parameter(collectionBase + static_cast<uint32_t>(at - collectionParameters.begin()), c);
    }

    static constexpr float nearPlane = 1.0f;

    std::optional<uint32_t> ownRecordWord(const ExprId index) {
        const ExprId primitive = graph.geometry(GeometryAttribute::PrimitiveId, 0);
        ExprId record = index;
        float word = 0.0f;
        for (ExprNode sum = graph.node(record); sum.op == ExprOp::Add; sum = graph.node(record)) {
            const bool constantFirst = graph.isConstant(sum.a);
            if (!constantFirst && !graph.isConstant(sum.b))
                break;
            word += graph.node(constantFirst ? sum.a : sum.b).value;
            record = constantFirst ? sum.b : sum.a;
        }
        const ExprNode& start = graph.node(record);
        const bool scaled =
            start.op == ExprOp::Mul && ((start.a == primitive && graph.isConstant(start.b)) || (start.b == primitive && graph.isConstant(start.a)));
        if ((record != primitive && !scaled) || word < 0.0f)
            return std::nullopt;
        return static_cast<uint32_t>(word);
    }

    std::array<ExprId, 4> transformRow(const GeometryAttribute firstAxis, const GeometryAttribute origin, const uint32_t r) {
        std::array<ExprId, 4> result{};
        for (uint32_t c = 0; c < 3; ++c)
            result[c] = r < 3 ? graph.geometry(static_cast<GeometryAttribute>(static_cast<uint32_t>(firstAxis) + r), c) : graph.geometry(origin, c);
        result[3] = graph.constant(r < 3 ? 0.0f : 1.0f);
        return result;
    }
    std::array<ExprId, 4> identityRow(const uint32_t r) {
        std::array<ExprId, 4> result{};
        for (uint32_t c = 0; c < 4; ++c)
            result[c] = graph.constant(c == r ? 1.0f : 0.0f);
        return result;
    }
    std::array<ExprId, 4> localToWorld(const uint32_t r) {
        return vertex ? identityRow(r) : transformRow(GeometryAttribute::ObjectAxisX, GeometryAttribute::ObjectOrigin, r);
    }
    std::array<ExprId, 4> worldToLocal(const uint32_t r) {
        return vertex ? identityRow(r) : transformRow(GeometryAttribute::WorldAxisX, GeometryAttribute::WorldOrigin, r);
    }

    ExprId cameraOrigin(const uint32_t c) { return graph.geometry(GeometryAttribute::CameraPosition, c); }
    ExprId translatedOrigin(const uint32_t c) { return vertex ? graph.constant(0.0f) : cameraOrigin(c); }
    ExprId translatedCameraOrigin(const uint32_t c) { return graph.sub(cameraOrigin(c), translatedOrigin(c)); }
    ExprId cameraForward(const uint32_t c) { return graph.geometry(GeometryAttribute::CameraForward, c); }
    ExprId cameraAxis(const uint32_t axis, const uint32_t c) {
        static constexpr GeometryAttribute axes[] = {GeometryAttribute::CameraRight, GeometryAttribute::CameraUp, GeometryAttribute::CameraForward};
        return graph.geometry(axes[axis], c);
    }
    std::array<ExprId, 4> viewToWorld(const uint32_t r) {
        std::array<ExprId, 4> result{};
        for (uint32_t c = 0; c < 3; ++c)
            result[c] = r < 3 ? cameraAxis(r, c) : translatedCameraOrigin(c);
        result[3] = graph.constant(r < 3 ? 0.0f : 1.0f);
        return result;
    }
    std::array<ExprId, 4> worldToView(const uint32_t r) {
        std::array<ExprId, 4> result{};
        for (uint32_t axis = 0; axis < 3; ++axis) {
            if (r < 3) {
                result[axis] = cameraAxis(axis, r);
                continue;
            }
            ExprId distance = graph.constant(0.0f);
            for (uint32_t c = 0; c < 3; ++c)
                distance = graph.add(distance, graph.mul(translatedCameraOrigin(c), cameraAxis(axis, c)));
            result[axis] = graph.negate(distance);
        }
        result[3] = graph.constant(r < 3 ? 0.0f : 1.0f);
        return result;
    }
    ExprId viewCoordinate(const uint32_t axis) {
        ExprId result = graph.constant(0.0f);
        for (uint32_t c = 0; c < 3; ++c)
            result = graph.add(result, graph.mul(graph.sub(position(c), cameraOrigin(c)), cameraAxis(axis, c)));
        return result;
    }
    ExprId viewDepth() { return viewCoordinate(2); }

    ExprId filmScale(const uint32_t c) { return graph.geometry(GeometryAttribute::CameraFilmScale, c); }
    ExprId viewSize(const uint32_t c) { return graph.geometry(GeometryAttribute::ViewSize, c); }

    ExprId viewToClip(const uint32_t r, const uint32_t c) {
        if (r == c && r < 2)
            return graph.div(graph.constant(1.0f), filmScale(r));
        if (r == 2 && c == 3)
            return graph.constant(1.0f);
        if (r == 3 && c == 2)
            return graph.constant(nearPlane);
        return graph.constant(0.0f);
    }

    ExprId clipToView(const uint32_t r, const uint32_t c) {
        if (r == c && r < 2)
            return filmScale(r);
        if (r == 2 && c == 3)
            return graph.constant(1.0f / nearPlane);
        if (r == 3 && c == 2)
            return graph.constant(1.0f);
        return graph.constant(0.0f);
    }

    ExprId worldToClip(const uint32_t r, const uint32_t c) {
        if (r < 3)
            return translatedWorldToClip(r, c);
        ExprId result = translatedWorldToClip(3, c);
        for (uint32_t k = 0; k < 3; ++k)
            result = graph.sub(result, graph.mul(translatedOrigin(k), translatedWorldToClip(k, c)));
        return result;
    }

    ExprId objectToClip(const uint32_t r, const uint32_t c) {
        const std::array<ExprId, 4> row = transformRow(GeometryAttribute::ObjectAxisX, GeometryAttribute::ObjectOrigin, r);
        ExprId result = graph.constant(0.0f);
        for (uint32_t k = 0; k < 4; ++k)
            result = graph.add(result, graph.mul(row[k], translatedWorldToClip(k, c)));
        return result;
    }

    ExprId translatedWorldToClip(const uint32_t r, const uint32_t c) {
        const std::array<ExprId, 4> row = worldToView(r);
        ExprId result = graph.constant(0.0f);
        for (uint32_t k = 0; k < 4; ++k)
            result = graph.add(result, graph.mul(row[k], viewToClip(k, c)));
        return result;
    }

    //! SPIR-V's FragCoord.w is 1 / view depth, where HLSL's SV_Position holds the depth: the cooked shaders take its reciprocal.
    ExprId svPosition(const uint32_t c) {
        const ExprId depth = viewDepth();
        if (c == 3)
            return graph.div(graph.constant(1.0f), depth);
        if (c == 2)
            return graph.div(graph.constant(nearPlane), depth);
        const ExprId half = graph.constant(0.5f);
        const ExprId device = graph.mul(graph.div(viewCoordinate(c), graph.mul(depth, filmScale(c))), half);
        return graph.mul(c == 0 ? graph.add(half, device) : graph.sub(half, device), viewSize(c));
    }

    ExprId svPositionToWorld(const uint32_t r, const uint32_t c) {
        if (c == 3)
            return graph.constant(r == 2 ? 1.0f / nearPlane : 0.0f);
        const auto perPixel = [&](const uint32_t axis) { return graph.div(graph.mul(graph.constant(2.0f), filmScale(axis)), viewSize(axis)); };
        switch (r) {
        case 0:
            return graph.mul(cameraAxis(0, c), perPixel(0));
        case 1:
            return graph.negate(graph.mul(cameraAxis(1, c), perPixel(1)));
        case 2:
            return graph.div(translatedCameraOrigin(c), graph.constant(nearPlane));
        default:
            return graph.add(graph.sub(cameraAxis(2, c), graph.mul(filmScale(0), cameraAxis(0, c))), graph.mul(filmScale(1), cameraAxis(1, c)));
        }
    }

    std::optional<std::array<ExprId, 4>> primitiveData(const uint32_t element) {
        const ExprId zero = graph.constant(0.0f), one = graph.constant(1.0f);
        if (element < 4)
            return localToWorld(element);
        if (element >= 6 && element < 10)
            return worldToLocal(element - 6);
        if (element >= 10 && element < 14)
            return localToWorld(element - 10);
        if (element >= 14 && element < 18)
            return worldToLocal(element - 14);

        std::array<ExprId, 3> scale{};
        std::array<std::array<ExprId, 4>, 3> axes{};
        for (uint32_t r = 0; r < 3; ++r) {
            axes[r] = localToWorld(r);
            scale[r] = graph.unary(
                ExprOp::Sqrt, graph.add(graph.add(graph.mul(axes[r][0], axes[r][0]), graph.mul(axes[r][1], axes[r][1])), graph.mul(axes[r][2], axes[r][2])));
        }
        if (element == 4) {
            const auto cross = [&](const int i, const int j) { return graph.sub(graph.mul(axes[1][i], axes[2][j]), graph.mul(axes[1][j], axes[2][i])); };
            const ExprId determinant =
                graph.add(graph.add(graph.mul(axes[0][0], cross(1, 2)), graph.mul(axes[0][1], cross(2, 0))), graph.mul(axes[0][2], cross(0, 1)));
            return std::array{graph.div(one, scale[0]), graph.div(one, scale[1]), graph.div(one, scale[2]), graph.unary(ExprOp::Sign, determinant)};
        }
        std::array<ExprId, 3> localMin{}, localMax{}, worldExtent{};
        for (uint32_t c = 0; c < 3; ++c) {
            localMin[c] = graph.geometry(GeometryAttribute::ObjectBoundsMin, c);
            localMax[c] = graph.geometry(GeometryAttribute::ObjectBoundsMax, c);
        }
        for (uint32_t c = 0; c < 3; ++c) {
            worldExtent[c] = graph.constant(0.0f);
            for (uint32_t r = 0; r < 3; ++r) {
                const ExprId half = graph.mul(graph.sub(localMax[r], localMin[r]), graph.constant(0.5f));
                worldExtent[c] = graph.add(worldExtent[c], graph.mul(graph.unary(ExprOp::Abs, axes[r][c]), half));
            }
        }
        if (element == 5) {
            std::array<ExprId, 4> result{};
            const auto origin = localToWorld(3);
            for (uint32_t c = 0; c < 3; ++c) {
                result[c] = origin[c];
                for (uint32_t r = 0; r < 3; ++r)
                    result[c] = graph.add(result[c], graph.mul(axes[r][c], graph.mul(graph.add(localMin[r], localMax[r]), graph.constant(0.5f))));
            }
            result[3] = graph.unary(ExprOp::Sqrt, graph.add(graph.add(graph.mul(worldExtent[0], worldExtent[0]), graph.mul(worldExtent[1], worldExtent[1])),
                                                            graph.mul(worldExtent[2], worldExtent[2])));
            return result;
        }
        if (element == 19)
            return std::array{worldExtent[0], worldExtent[1], worldExtent[2], zero};
        if (element == 20)
            return std::array{zero, zero, zero, zero};
        if (element == 23)
            return std::array{localMin[0], localMin[1], localMin[2], zero};
        if (element == 24)
            return std::array{localMax[0], localMax[1], localMax[2], zero};
        if (element == 25)
            return std::array{localMin[0], localMin[1], localMin[2], zero};
        if (element == 26)
            return std::array{localMax[0], localMax[1], localMax[2], zero};
        if (element == 22)
            return std::array{scale[0], scale[1], scale[2], graph.binary(ExprOp::Max, scale[0], graph.binary(ExprOp::Max, scale[1], scale[2]))};
        return std::nullopt;
    }

    std::optional<ExprId> primitiveField(const std::string& name, const uint32_t word) {
        struct Field {
            uint32_t element, first, count;
        };
        static const std::map<std::string, Field> fields = {
            {"Primitive_InvNonUniformScale", {4, 0, 3}},
            {"Primitive_ObjectWorldPositionAndRadius", {5, 0, 4}},
            {"Primitive_ObjectRelativeWorldPositionAndRadius", {5, 0, 4}},
            {"Primitive_ObjectBoundsX", {19, 0, 1}},
            {"Primitive_ObjectBoundsY", {19, 1, 1}},
            {"Primitive_ObjectBoundsZ", {19, 2, 1}},
            {"Primitive_NonUniformScale", {22, 0, 4}},
            {"Primitive_LocalObjectBoundsMin", {23, 0, 3}},
            {"Primitive_LocalObjectBoundsMax", {24, 0, 3}},
            {"Primitive_PreSkinnedLocalBoundsMin", {25, 0, 3}},
            {"Primitive_PreSkinnedLocalBoundsMax", {26, 0, 3}},
        };
        const auto field = fields.find(name);
        if (field == fields.end() || word >= field->second.count)
            return std::nullopt;
        return (*primitiveData(field->second.element))[field->second.first + word];
    }

    ExprId materialWord(const std::string& name, const uint32_t word) {
        return word < material.size() ? material[word] : graph.unresolved(name + "[" + std::to_string(word) + "]");
    }

    ExprId failed(std::string message) {
        graph.fail(std::move(message));
        return graph.constant(0.0f);
    }

    ExprId unknown(const std::string& what) {
        absentValueRead = true;
        if (absentValues == AbsentValues::ReadAsZero)
            return graph.constant(0.0f);
        return graph.unresolved(shader + ": " + what + "; it reads as zero");
    }

    ExprId objectOrientation(const uint32_t c) {
        const std::array<ExprId, 4> axis = localToWorld(2);
        const ExprId length =
            graph.unary(ExprOp::Sqrt, graph.add(graph.add(graph.mul(axis[0], axis[0]), graph.mul(axis[1], axis[1])), graph.mul(axis[2], axis[2])));
        return graph.div(axis[c], length);
    }

    ExprId direction(const GeometryAttribute attribute, const uint32_t c) { return graph.geometry(attribute, c); }
    ExprId position(const uint32_t c) { return graph.geometry(GeometryAttribute::Position, c); }

    ExprId tangentSign() {
        ExprId n[3], t[3], b[3];
        for (uint32_t i = 0; i < 3; ++i) {
            n[i] = direction(GeometryAttribute::Normal, i);
            t[i] = direction(GeometryAttribute::Tangent, i);
            b[i] = direction(GeometryAttribute::Bitangent, i);
        }
        const auto cross = [&](int i, int j) { return graph.sub(graph.mul(n[i], t[j]), graph.mul(n[j], t[i])); };
        const ExprId dot = graph.add(graph.add(graph.mul(cross(1, 2), b[0]), graph.mul(cross(2, 0), b[1])), graph.mul(cross(0, 1), b[2]));
        return graph.unary(ExprOp::Sign, dot);
    }

    ExpressionGraph& graph;
    const AbsentValues absentValues;
    bool& absentValueRead;
    const std::string& shader;
    const ShaderReflection& reflection;
    const std::vector<ExprId>& material;
    const TextureSlots& textureSlots;
    const EngineValues& engine;
    std::span<const ParameterCollection> collections;
    std::vector<NumericParameter>& collectionParameters;
    const EngineSymbols& symbols;
    const Interpolants& interpolants;
    const bool vertex;
    const bool skinned;
    const VertexFactory factory;
    const uint32_t particleBase;
    const VertexProjection projection;
    const uint32_t collectionBase;
};

}

MaterialEvaluator::MaterialEvaluator(const MaterialData& data, const AbsentValues absentValues)
    : data(data), absentValues(absentValues), material(materialUniforms(graph, data, warnings)) {
    uint32_t programTexture = 0;
    for (const TextureSlot& slot : data.textures)
        if (programSamples(slot.type))
            textureSlots[{slot.type, static_cast<uint32_t>(slot.index)}] = {programTexture++, slot.size};
}

ShaderOutputs MaterialEvaluator::evaluate(const CompiledMaterialShader& shader, const EngineValues& engine, const EngineSymbols& symbols,
                                          const VertexProjection projection) {
    for (const std::string& problem : shader.reflection.getProblems())
        warnings.add(shader.type + ": " + problem);
    UnrealShaderInputs inputs(graph, absentValues, absentValueRead, shader, material, textureSlots, engine, data.parameterCollections, collectionParameters, symbols,
                              interpolants, data.vertexFactory, static_cast<uint32_t>(data.parameters.size()), projection);
    ShaderOutputs outputs = evaluateSpirv(*shader.spirv, graph, inputs);
    inputs.resolveProjection(outputs);
    for (const std::string& note : outputs.notes)
        warnings.add(shader.type + ": " + note);
    if (shader.stage != ShaderStage::Vertex)
        return outputs;
    interpolants.clear();
    for (const ShaderReflection::SignatureElement& element : shader.reflection.getOutputs()) {
        if (element.reg >= outputs.targets.size())
            continue;
        std::vector<ExprId>& values = interpolants[{element.semantic, element.semanticIndex}];
        for (uint32_t c = 0; c < 4; ++c)
            if ((element.mask & (1u << c)) != 0)
                values.push_back(outputs.targets[element.reg][c]);
    }
    return outputs;
}

EngineValues viewDefaults() {
    return {
        {"View_NormalOverrideParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"View_RoughnessOverrideParameter", {0.0f, 1.0f}},
        {"View_DiffuseOverrideParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"View_SpecularOverrideParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"View_MaterialTextureMipBias", {0.0f}},
        {"View_MaterialTextureDerivativeMultiply", {1.0f}},
        {"View_UnlitViewmodeMask", {0.0f}},
        {"View_CullingSign", {1.0f}},
        {"View_PreExposure", {1.0f}},
        {"View_OneOverPreExposure", {1.0f}},
        {"View_OutOfBoundsMask", {0.0f}},
        {"View_NumSceneColorMSAASamples", {1.0f}},
        {"View_ShowDecalsMask", {1.0f}},
        {"View_HMDEyePaddingOffset", {1.0f}},
        {"View_ForceDrawAllVelocities", {0.0f}},
        {"View_MatrixTilePosition", {0.0f, 0.0f, 0.0f, 0.0f}},
        {"View_TemporalAAParams", {0.0f, 1.0f, 0.0f, 0.0f}},
        {"View_StateFrameIndex", {0.0f}},
        {"PrimitiveFade_FadeTimeScaleBias", {0.0f, 1.0f}},
        {"View_SkyLightColor", {0.0f, 0.0f, 0.0f, 0.0f}},
        {"SceneColor", {0.0f}},
        {"MobileBasePass_Fog_ExponentialFogColorParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"TranslucentBasePass_Shared_Fog_ExponentialFogColorParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"TranslucentBasePass_Shared_FogISR_ExponentialFogColorParameter", {0.0f, 0.0f, 0.0f, 1.0f}},
        {"View_ViewRectMin", {0.0f, 0.0f, 0.0f, 0.0f}},
        {"Primitive_Flags", {std::bit_cast<float>(0x8000u)}},
        {"Primitive_MaxWPOExtent", {0.0f}},
        {"Primitive_FOVProjectionMatrixIndex", {0.0f}},
        //! The game sets stencil bit 8 on the world pixels that no agent covers; AresStencil effects draw only there.
        {"MobileBasePass_SceneTextures_AresStencilTexture", {0.0f, std::bit_cast<float>(8u), 0.0f, 0.0f}},
        //! With a zero stride, a vertex fetch index is the number of the element.
        {"LocalVF_VertexFetch_Parameters", {0.0f, 0.0f, 0.0f, 0.0f}},
    };
}

}
