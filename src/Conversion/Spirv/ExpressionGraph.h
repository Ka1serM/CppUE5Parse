#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ue5parse::conversion {

using ExprId = uint32_t;

enum class SemanticOp : uint8_t { Clamp, Mix, Smoothstep, Length, Distance, Normalize, Cross, Reflect };

struct SemanticExpression {
    SemanticOp op;
    uint32_t outputCount = 0;
    std::vector<ExprId> a, b, c;
};

enum class ExprOp : uint8_t {
    Constant,
    Parameter,
    Texture,
    Geometry,
    GeometryDx,
    GeometryDy,
    Time,
    Frame,
    Unresolved,
    Add,
    Sub,
    Mul,
    Div,
    Min,
    Max,
    Pow,
    Atan2,
    Mod,
    Abs,
    Floor,
    Ceil,
    Fract,
    Round,
    Trunc,
    Sign,
    Sqrt,
    Exp2,
    Log2,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Random,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    And,
    Or,
    Not,
    Select,
};

enum class GeometryAttribute : uint8_t {
    Position,
    Normal,
    Tangent,
    Bitangent,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    VertexColor,
    ObjectAxisX,
    ObjectAxisY,
    ObjectAxisZ,
    ObjectOrigin,
    WorldAxisX,
    WorldAxisY,
    WorldAxisZ,
    WorldOrigin,
    ObjectBoundsMin,
    ObjectBoundsMax,
    CameraPosition,
    CameraForward,
    CameraRight,
    CameraUp,
    SceneDepth,
    ViewDirection,
    PrimitiveId,
    CameraFilmScale,
    ViewSize,
    CustomData0,
    CustomData1,
    CustomData2,
    CustomData3,
    CustomData4,
    CustomData5,
    CustomData6,
    CustomData7,
    CustomData8,
    LocalPosition,
};

enum class ScreenAxis : uint8_t { X, Y };

struct ExprNode {
    ExprOp op = ExprOp::Constant;
    uint32_t index = 0;
    uint32_t component = 0;
    float value = 0.0f;
    ExprId a = 0, b = 0, c = 0;

    bool operator==(const ExprNode&) const = default;
};

class ExpressionGraph {
public:
    ExpressionGraph();

    const ExprNode& node(ExprId id) const { return nodes[id]; }
    size_t size() const { return nodes.size(); }

    ExprId constant(float value);
    ExprId boolean(bool value) { return value ? trueId : falseId; }
    ExprId parameter(uint32_t parameter, uint32_t component);
    ExprId texture(uint32_t slot, uint32_t component, ExprId u, ExprId v);
    ExprId geometry(GeometryAttribute attribute, uint32_t component);
    ExprId geometryDerivative(GeometryAttribute attribute, uint32_t component, ScreenAxis axis);
    ExprId time();
    ExprId frame();
    ExprId unresolved(const std::string& name);
    const std::string& unresolvedName(ExprId id) const { return unresolvedNames[nodes[id].index]; }

    ExprId unary(ExprOp op, ExprId a);
    ExprId binary(ExprOp op, ExprId a, ExprId b);
    ExprId select(ExprId condition, ExprId a, ExprId b);
    ExprId screenDerivative(ExprId value, ScreenAxis axis);

    ExprId add(ExprId a, ExprId b) { return binary(ExprOp::Add, a, b); }
    ExprId sub(ExprId a, ExprId b) { return binary(ExprOp::Sub, a, b); }
    ExprId mul(ExprId a, ExprId b) { return binary(ExprOp::Mul, a, b); }
    ExprId div(ExprId a, ExprId b) { return binary(ExprOp::Div, a, b); }
    ExprId min(ExprId a, ExprId b) { return binary(ExprOp::Min, a, b); }
    ExprId max(ExprId a, ExprId b) { return binary(ExprOp::Max, a, b); }
    ExprId clamp(ExprId x, ExprId low, ExprId high) { return min(max(x, low), high); }
    ExprId negate(ExprId a) { return sub(constant(0.0f), a); }
    ExprId less(ExprId a, ExprId b) { return binary(ExprOp::Less, a, b); }
    ExprId logicalNot(ExprId a) { return unary(ExprOp::Not, a); }
    ExprId logicalAnd(ExprId a, ExprId b) { return binary(ExprOp::And, a, b); }
    ExprId logicalOr(ExprId a, ExprId b) { return binary(ExprOp::Or, a, b); }

    bool isConstant(ExprId id) const { return nodes[id].op == ExprOp::Constant; }
    bool isConstant(ExprId id, float value) const { return isConstant(id) && nodes[id].value == value; }
    bool isBoolean(ExprId id) const;
    void annotate(std::span<const ExprId> outputs, SemanticExpression expression);
    std::optional<std::pair<uint32_t, uint32_t>> semanticOutput(ExprId id) const;
    const SemanticExpression& semanticExpression(uint32_t id) const { return semanticExpressions.at(id); }
    std::vector<ExprId> unresolvedInputs(ExprId root) const;
    std::optional<ExprId> coefficient(ExprId value, ExprId leaf);

    void fail(std::string message);
    bool isFailed() const { return failure.has_value(); }
    const std::optional<std::string>& getFailure() const { return failure; }

private:
    struct ExprNodeHash {
        size_t operator()(const ExprNode& node) const;
    };

    ExprId intern(const ExprNode& node);
    ExprId fold(ExprOp op, ExprId a, ExprId b, ExprId c);
    std::optional<ExprId> reassociate(ExprOp op, ExprId a, ExprId b);
    ExprId shifted(ExprId value, ScreenAxis axis);

    std::vector<ExprNode> nodes;
    std::unordered_map<ExprNode, ExprId, ExprNodeHash> index;
    std::vector<std::string> unresolvedNames;
    std::unordered_map<std::string, uint32_t> unresolvedIndices;
    std::vector<SemanticExpression> semanticExpressions;
    std::unordered_map<ExprId, std::pair<uint32_t, uint32_t>> semanticOutputs;
    ExprId trueId = 0, falseId = 0;
    std::unordered_map<ExprId, ExprId> shiftedCache[2];
    std::optional<std::string> failure;
};

}
