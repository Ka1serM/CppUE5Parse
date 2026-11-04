#include "Conversion/Spirv/ProgramWriter.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <numeric>

namespace ue5parse::conversion {
namespace {

uint32_t geometryWidth(const GeometryAttribute attribute) {
    if ((attribute >= GeometryAttribute::TexCoord0 && attribute <= GeometryAttribute::TexCoord7) || attribute == GeometryAttribute::CameraFilmScale
        || attribute == GeometryAttribute::ViewSize)
        return 2;
    if (attribute == GeometryAttribute::VertexColor || (attribute >= GeometryAttribute::CustomData0 && attribute <= GeometryAttribute::CustomData8))
        return 4;
    if (attribute == GeometryAttribute::SceneDepth || attribute == GeometryAttribute::PrimitiveId)
        return 1;
    return 3;
}

bool commutative(const ExprOp op) {
    return op == ExprOp::Add || op == ExprOp::Mul || op == ExprOp::Min || op == ExprOp::Max;
}

bool commutative(const ProgramOp op) {
    return op == ProgramOp::Add || op == ProgramOp::Mul || op == ProgramOp::Min || op == ProgramOp::Max || op == ProgramOp::Equal || op == ProgramOp::NotEqual
           || op == ProgramOp::Dot || op == ProgramOp::Distance;
}

std::optional<ProgramOp> arithmetic(const ExprOp op) {
    switch (op) {
    case ExprOp::Add:
        return ProgramOp::Add;
    case ExprOp::Sub:
        return ProgramOp::Sub;
    case ExprOp::Mul:
        return ProgramOp::Mul;
    case ExprOp::Div:
        return ProgramOp::Div;
    case ExprOp::Min:
        return ProgramOp::Min;
    case ExprOp::Max:
        return ProgramOp::Max;
    case ExprOp::Pow:
        return ProgramOp::Pow;
    case ExprOp::Atan2:
        return ProgramOp::Atan2;
    case ExprOp::Mod:
        return ProgramOp::Mod;
    case ExprOp::Abs:
        return ProgramOp::Abs;
    case ExprOp::Floor:
        return ProgramOp::Floor;
    case ExprOp::Ceil:
        return ProgramOp::Ceil;
    case ExprOp::Fract:
        return ProgramOp::Fract;
    case ExprOp::Round:
        return ProgramOp::Round;
    case ExprOp::Trunc:
        return ProgramOp::Trunc;
    case ExprOp::Sign:
        return ProgramOp::Sign;
    case ExprOp::Sqrt:
        return ProgramOp::Sqrt;
    case ExprOp::Exp2:
        return ProgramOp::Exp2;
    case ExprOp::Log2:
        return ProgramOp::Log2;
    case ExprOp::Sin:
        return ProgramOp::Sin;
    case ExprOp::Cos:
        return ProgramOp::Cos;
    case ExprOp::Tan:
        return ProgramOp::Tan;
    case ExprOp::Asin:
        return ProgramOp::Asin;
    case ExprOp::Acos:
        return ProgramOp::Acos;
    case ExprOp::Atan:
        return ProgramOp::Atan;
    case ExprOp::Random:
        return ProgramOp::Random;
    default:
        return std::nullopt;
    }
}

bool unary(const ExprOp op) {
    return op >= ExprOp::Abs && op <= ExprOp::Random;
}

uint32_t operandCount(const ProgramInstruction& instruction) {
    switch (instruction.op) {
    case ProgramOp::Constant:
    case ProgramOp::Parameter:
    case ProgramOp::Geometry:
    case ProgramOp::GeometryDx:
    case ProgramOp::GeometryDy:
    case ProgramOp::Time:
    case ProgramOp::Frame:
        return 0;
    case ProgramOp::Combine:
        return instruction.width;
    case ProgramOp::Texture:
    case ProgramOp::Extract:
    case ProgramOp::Abs:
    case ProgramOp::Floor:
    case ProgramOp::Ceil:
    case ProgramOp::Fract:
    case ProgramOp::Round:
    case ProgramOp::Trunc:
    case ProgramOp::Sign:
    case ProgramOp::Sqrt:
    case ProgramOp::Exp2:
    case ProgramOp::Log2:
    case ProgramOp::Sin:
    case ProgramOp::Cos:
    case ProgramOp::Tan:
    case ProgramOp::Asin:
    case ProgramOp::Acos:
    case ProgramOp::Atan:
    case ProgramOp::Random:
    case ProgramOp::Length:
    case ProgramOp::Normalize:
    case ProgramOp::TangentToWorld:
        return 1;
    case ProgramOp::Select:
    case ProgramOp::Clamp:
    case ProgramOp::Mix:
    case ProgramOp::Smoothstep:
    case ProgramOp::MulAdd:
        return 3;
    default:
        return 2;
    }
}

bool vectorizes(const ExprOp op) {
    switch (op) {
    case ExprOp::Add:
    case ExprOp::Sub:
    case ExprOp::Mul:
    case ExprOp::Div:
    case ExprOp::Min:
    case ExprOp::Max:
    case ExprOp::Mod:
    case ExprOp::Abs:
    case ExprOp::Floor:
    case ExprOp::Ceil:
    case ExprOp::Fract:
    case ExprOp::Sin:
    case ExprOp::Cos:
    case ExprOp::Tan:
        return true;
    default:
        return false;
    }
}

}

ProgramWriter::ProgramWriter(const ExpressionGraph& graph, const std::span<const uint32_t> parameterWidths)
    : graph(graph), parameterWidths(parameterWidths.begin(), parameterWidths.end()) {}

uint32_t ProgramWriter::write(const ExprId value) {
    program.outputs.push_back(emit(value));
    return static_cast<uint32_t>(program.outputs.size() - 1);
}

uint32_t ProgramWriter::write(const std::span<const ExprId, 3> value) {
    program.outputs.push_back(emit(Lanes(value.begin(), value.end())));
    return static_cast<uint32_t>(program.outputs.size() - 1);
}

uint32_t ProgramWriter::instruction(const ProgramOp op, const uint32_t width, const uint32_t index, uint32_t a, uint32_t b, const uint32_t c) {
    if (commutative(op) && a > b)
        std::swap(a, b);
    if (const std::optional<uint32_t> simpler = simplify(op, width, a, b, c))
        return *simpler;
    if (index > std::numeric_limits<uint16_t>::max()) {
        if (!failure)
            failure = "The program has more leaves than an instruction can index.";
        return 0;
    }
    const Key key{op, static_cast<uint8_t>(width), static_cast<uint16_t>(index), a, b, c};
    if (const auto found = instructions.find(key); found != instructions.end())
        return found->second;
    const auto id = static_cast<uint32_t>(program.instructions.size());
    program.instructions.push_back({op, static_cast<uint8_t>(width), static_cast<uint16_t>(index), a, b, c});
    instructions.emplace(key, id);
    return id;
}

std::vector<uint32_t> ProgramWriter::uses() const {
    const std::vector<ProgramInstruction>& all = program.instructions;
    std::vector<uint32_t> counts(all.size());
    for (const uint32_t output : program.outputs)
        ++counts[output];
    for (size_t i = all.size(); i-- > 0;) {
        if (counts[i] == 0)
            continue;
        const std::array<uint32_t, 3> operands{all[i].a, all[i].b, all[i].c};
        for (uint32_t k = 0; k < operandCount(all[i]); ++k)
            ++counts[operands[k]];
    }
    return counts;
}

ExpressionProgram ProgramWriter::finish() {
    std::vector<ProgramInstruction>& all = program.instructions;
    const std::vector<uint32_t> before = uses();
    for (ProgramInstruction& sum : all) {
        if (sum.op != ProgramOp::Add)
            continue;
        for (const auto [product, addend] : {std::pair{sum.a, sum.b}, std::pair{sum.b, sum.a}}) {
            if (all[product].op != ProgramOp::Mul || before[product] != 1)
                continue;
            sum = {ProgramOp::MulAdd, sum.width, 0, all[product].a, all[product].b, addend};
            break;
        }
    }

    const std::vector<uint32_t> used = uses();
    std::vector<uint32_t> moved(all.size());
    uint32_t count = 0;
    for (size_t i = 0; i < all.size(); ++i) {
        if (used[i] == 0)
            continue;
        const ProgramInstruction& old = all[i];
        std::array<uint32_t, 3> operands{old.a, old.b, old.c};
        for (uint32_t k = 0; k < operandCount(old); ++k)
            operands[k] = moved[operands[k]];
        all[count] = {old.op, old.width, old.index, operands[0], operands[1], operands[2]};
        moved[i] = count++;
    }
    all.resize(count);
    for (uint32_t& output : program.outputs)
        output = moved[output];
    instructions.clear();
    return std::move(program);
}

uint32_t ProgramWriter::constant(const std::span<const float> values) {
    std::vector<float> key(values.begin(), values.end());
    if (const auto found = constants.find(key); found != constants.end())
        return found->second;
    const auto offset = static_cast<uint32_t>(program.constants.size());
    program.constants.insert(program.constants.end(), values.begin(), values.end());
    const uint32_t id = instruction(ProgramOp::Constant, static_cast<uint32_t>(values.size()), offset);
    constants.emplace(std::move(key), id);
    return id;
}

uint32_t ProgramWriter::extract(const uint32_t vector, const uint32_t component) {
    if (width(vector) == 1)
        return vector;
    const ProgramInstruction& source = program.instructions[vector];
    if (source.op == ProgramOp::Constant)
        return constant(program.constants[source.index + component]);
    if (source.op == ProgramOp::Combine)
        return component == 0 ? source.a : component == 1 ? source.b : source.c;
    return instruction(ProgramOp::Extract, 1, component, vector);
}

uint32_t ProgramWriter::emit(const ExprId id) {
    if (const auto found = emitted.find(id); found != emitted.end())
        return found->second;
    if (const auto semantic = graph.semanticOutput(id); semantic && !semanticActive.contains(semantic->first))
        if (const std::optional<uint32_t> result = emitSemantic(semantic->first))
            return emitted.try_emplace(id, extract(*result, semantic->second)).first->second;
    if (const auto found = vectorComponents.find(id); found != vectorComponents.end())
        return emitted.try_emplace(id, extract(found->second.first, found->second.second)).first->second;
    const uint32_t result = emitNode(id);
    return emitted.try_emplace(id, result).first->second;
}

std::optional<uint32_t> ProgramWriter::emitSemantic(const uint32_t index) {
    if (const auto found = semanticResults.find(index); found != semanticResults.end())
        return found->second;
    const SemanticExpression& expression = graph.semanticExpression(index);
    const bool fits = expression.outputCount <= 3 && expression.a.size() <= 3 && expression.b.size() <= 3 && expression.c.size() <= 3;
    if (!fits)
        return semanticResults.emplace(index, std::nullopt).first->second;
    semanticActive.insert(index);
    struct Leave {
        std::unordered_set<uint32_t>& active;
        uint32_t index;
        ~Leave() { active.erase(index); }
    } leave{semanticActive, index};
    const auto input = [&](const Lanes& values) { return values.size() == 1 ? emit(values.front()) : emit(values); };
    const uint32_t outputs = expression.outputCount;
    uint32_t result = 0;
    switch (expression.op) {
    case SemanticOp::Clamp:
        result = instruction(ProgramOp::Clamp, outputs, 0, input(expression.a), input(expression.b), input(expression.c));
        break;
    case SemanticOp::Mix:
        result = instruction(ProgramOp::Mix, outputs, 0, input(expression.a), input(expression.b), input(expression.c));
        break;
    case SemanticOp::Smoothstep:
        result = instruction(ProgramOp::Smoothstep, outputs, 0, input(expression.a), input(expression.b), input(expression.c));
        break;
    case SemanticOp::Length:
        result = instruction(ProgramOp::Length, 1, 0, input(expression.a));
        break;
    case SemanticOp::Distance:
        result = instruction(ProgramOp::Distance, 1, 0, input(expression.a), input(expression.b));
        break;
    case SemanticOp::Normalize:
        result = instruction(ProgramOp::Normalize, outputs, 0, input(expression.a));
        break;
    case SemanticOp::Cross:
        result = instruction(ProgramOp::Cross, 3, 0, input(expression.a), input(expression.b));
        break;
    case SemanticOp::Reflect:
        result = instruction(ProgramOp::Reflect, 3, 0, input(expression.a), input(expression.b));
        break;
    }
    return semanticResults.emplace(index, result).first->second;
}

std::optional<uint32_t> ProgramWriter::semanticLanes(const Lanes& lanes) const {
    std::optional<uint32_t> index;
    for (size_t component = 0; component < lanes.size(); ++component) {
        const auto output = graph.semanticOutput(lanes[component]);
        if (!output || output->second != component || (index && *index != output->first))
            return std::nullopt;
        index = output->first;
    }
    return index;
}

uint32_t ProgramWriter::rememberVectorLanes(const Lanes& lanes, const uint32_t vector) {
    for (size_t i = 0; i < lanes.size(); ++i)
        if (!emitted.contains(lanes[i]))
            vectorComponents.try_emplace(lanes[i], vector, static_cast<uint32_t>(i));
    return vector;
}

uint32_t ProgramWriter::leaf(const ExprNode& node) {
    switch (node.op) {
    case ExprOp::Parameter:
        return instruction(ProgramOp::Parameter, parameterWidths.at(node.index), node.index);
    case ExprOp::Texture:
        return instruction(ProgramOp::Texture, 4, node.index, emit(Lanes{node.a, node.b}));
    case ExprOp::Geometry:
        return instruction(ProgramOp::Geometry, geometryWidth(static_cast<GeometryAttribute>(node.index)), node.index);
    case ExprOp::GeometryDx:
        return instruction(ProgramOp::GeometryDx, geometryWidth(static_cast<GeometryAttribute>(node.index)), node.index);
    case ExprOp::GeometryDy:
        return instruction(ProgramOp::GeometryDy, geometryWidth(static_cast<GeometryAttribute>(node.index)), node.index);
    default:
        return fail("A leaf of an unknown kind.");
    }
}

uint32_t ProgramWriter::emitNode(const ExprId id) {
    const ExprNode& node = graph.node(id);
    switch (node.op) {
    case ExprOp::Constant:
        if (graph.isBoolean(id))
            return fail("A boolean is used as a number.");
        return constant(node.value);
    case ExprOp::Parameter:
    case ExprOp::Texture:
    case ExprOp::Geometry:
    case ExprOp::GeometryDx:
    case ExprOp::GeometryDy:
        return extract(leaf(node), node.component);
    case ExprOp::Time:
        return instruction(ProgramOp::Time, 1);
    case ExprOp::Frame:
        return instruction(ProgramOp::Frame, 1);
    case ExprOp::Unresolved:
        return fail("The engine input " + graph.unresolvedName(id) + " reached the material.");
    case ExprOp::Select:
        return instruction(ProgramOp::Select, 1, 0, mask(node.a), emit(node.b), emit(node.c));
    case ExprOp::Mul:
        if (const std::optional<uint32_t> curve = smoothstep(id))
            return *curve;
        break;
    case ExprOp::Add:
        if (const auto dot = dotProduct(id)) {
            uint32_t result = instruction(ProgramOp::Dot, 1, 0, emit(dot->a), emit(dot->b));
            for (const ExprId factor : dot->scale)
                result = instruction(ProgramOp::Mul, 1, 0, result, emit(factor));
            return result;
        }
        break;
    case ExprOp::Sqrt:
        if (auto values = squares(node.a)) {
            std::ranges::sort(*values);
            Lanes ordered = *values;
            while (shape(ordered) == Shape::None && std::ranges::next_permutation(ordered).found) {
            }
            return instruction(ProgramOp::Length, 1, 0, emit(shape(ordered) == Shape::None ? *values : ordered));
        }
        break;
    default:
        break;
    }
    const std::optional<ProgramOp> op = arithmetic(node.op);
    if (!op)
        return fail("A boolean is used as a number.");
    if (unary(node.op))
        return instruction(*op, 1, 0, emit(node.a));
    return instruction(*op, 1, 0, emit(node.a), emit(node.b));
}

uint32_t ProgramWriter::emit(const Lanes& lanes) {
    if (const auto found = vectors.find(lanes); found != vectors.end())
        return found->second;
    const uint32_t result = emitLanes(lanes);
    vectors.emplace(lanes, result);
    return result;
}

uint32_t ProgramWriter::emitLanes(const Lanes& lanes) {
    const auto width = static_cast<uint32_t>(lanes.size());
    const auto combine = [&] {
        std::array<uint32_t, 3> components{};
        for (size_t i = 0; i < lanes.size(); ++i)
            components[i] = emit(lanes[i]);
        return instruction(ProgramOp::Combine, width, 0, components[0], components[1], components[2]);
    };
    switch (shape(lanes)) {
    case Shape::Uniform:
        return emit(lanes[0]);
    case Shape::Constant: {
        std::vector<float> values;
        for (const ExprId lane : lanes)
            values.push_back(graph.node(lane).value);
        return constant(values);
    }
    case Shape::Leaf:
        return leaf(graph.node(lanes[0]));
    case Shape::None:
        return combine();
    case Shape::Operation:
        break;
    }

    if (const auto semantic = semanticLanes(lanes); semantic && !semanticActive.contains(*semantic))
        if (const std::optional<uint32_t> result = emitSemantic(*semantic))
            return *result;

    if (std::ranges::all_of(lanes, [&](const ExprId lane) { return emitted.contains(lane) || vectorComponents.contains(lane); }))
        return combine();

    if (const auto factors = crossProduct(lanes))
        return rememberVectorLanes(lanes, instruction(ProgramOp::Cross, 3, 0, emit(factors->first), emit(factors->second)));
    if (const auto values = normalized(lanes))
        return rememberVectorLanes(lanes, instruction(ProgramOp::Normalize, width, 0, emit(*values)));
    const ExprNode& first = graph.node(lanes[0]);
    if (first.op == ExprOp::Min) {
        Lanes values, lows, highs;
        for (const ExprId lane : lanes) {
            auto clamped = clamp(lane);
            if (!clamped)
                break;
            if (!lows.empty() && clamped->low != lows[0] && clamped->value == lows[0])
                std::swap(clamped->value, clamped->low);
            values.push_back(clamped->value);
            lows.push_back(clamped->low);
            highs.push_back(clamped->high);
        }
        if (values.size() == lanes.size() && shape(values) != Shape::None && shape(lows) != Shape::None && shape(highs) != Shape::None)
            return rememberVectorLanes(lanes, instruction(ProgramOp::Clamp, width, 0, emit(values), operand(lows), operand(highs)));
    }
    const auto [a, b, c] = operands(lanes);
    if (first.op == ExprOp::Select)
        return rememberVectorLanes(lanes, instruction(ProgramOp::Select, width, 0, mask(first.a), operand(b), operand(c)));
    const ProgramOp op = *arithmetic(first.op);
    if (unary(first.op))
        return rememberVectorLanes(lanes, instruction(op, width, 0, emit(a)));
    return rememberVectorLanes(lanes, instruction(op, width, 0, operand(a), operand(b)));
}

uint32_t ProgramWriter::operand(const Lanes& lanes) {
    if (shape(lanes) == Shape::Uniform)
        return emit(lanes[0]);
    return emit(lanes);
}

ProgramWriter::Shape ProgramWriter::shape(const Lanes& lanes) {
    if (const auto found = shapes.find(lanes); found != shapes.end())
        return found->second;
    const Shape result = computeShape(lanes);
    shapes.emplace(lanes, result);
    return result;
}

ProgramWriter::Shape ProgramWriter::computeShape(const Lanes& lanes) {
    const auto all = [&](auto predicate) { return std::ranges::all_of(lanes, predicate); };
    if (all([&](const ExprId lane) { return graph.isBoolean(lane); }))
        return Shape::None;
    if (all([&](const ExprId lane) { return lane == lanes[0]; }))
        return Shape::Uniform;
    if (all([&](const ExprId lane) { return graph.isConstant(lane) && !graph.isBoolean(lane); }))
        return Shape::Constant;

    const ExprNode& first = graph.node(lanes[0]);
    bool leafLanes = first.op == ExprOp::Parameter || first.op == ExprOp::Texture || first.op == ExprOp::Geometry || first.op == ExprOp::GeometryDx
                     || first.op == ExprOp::GeometryDy;
    for (size_t i = 0; i < lanes.size() && leafLanes; ++i) {
        const ExprNode& lane = graph.node(lanes[i]);
        leafLanes = lane.op == first.op && lane.index == first.index && lane.a == first.a && lane.b == first.b && lane.component == i;
    }
    if (leafLanes) {
        if (first.op == ExprOp::Parameter)
            return parameterWidths.at(first.index) > 1 ? Shape::Leaf : Shape::None;
        if (first.op != ExprOp::Texture && geometryWidth(static_cast<GeometryAttribute>(first.index)) < std::max<size_t>(lanes.size(), 2))
            return Shape::None;
        return Shape::Leaf;
    }

    if (crossProduct(lanes) || normalized(lanes))
        return Shape::Operation;
    if (!all([&](const ExprId lane) { return graph.node(lane).op == first.op; }))
        return Shape::None;
    const auto [a, b, c] = operands(lanes);
    if (first.op == ExprOp::Select) {
        if (!all([&](const ExprId lane) { return graph.node(lane).a == first.a; }))
            return Shape::None;
        return shape(b) != Shape::None && shape(c) != Shape::None ? Shape::Operation : Shape::None;
    }
    if (!vectorizes(first.op))
        return Shape::None;
    if (shape(a) == Shape::None || (!unary(first.op) && shape(b) == Shape::None))
        return Shape::None;
    return Shape::Operation;
}

std::array<ProgramWriter::Lanes, 3> ProgramWriter::operands(const Lanes& lanes) const {
    const ExprNode& first = graph.node(lanes[0]);
    const auto similar = [&](const ExprId x, const ExprId y) {
        const ExprNode& p = graph.node(x);
        const ExprNode& q = graph.node(y);
        return x == y || (p.op == q.op && p.index == q.index);
    };
    std::array<Lanes, 3> result;
    for (const ExprId lane : lanes) {
        const ExprNode& node = graph.node(lane);
        ExprId a = node.a, b = node.b;
        if (commutative(node.op) && !similar(a, first.a) && similar(b, first.a))
            std::swap(a, b);
        result[0].push_back(a);
        result[1].push_back(b);
        result[2].push_back(node.c);
    }
    return result;
}

uint32_t ProgramWriter::mask(const ExprId condition) {
    const ExprNode& node = graph.node(condition);
    switch (node.op) {
    case ExprOp::Constant:
        return constant(node.value);
    case ExprOp::And:
        return instruction(ProgramOp::Mul, 1, 0, mask(node.a), mask(node.b));
    case ExprOp::Or:
        return instruction(ProgramOp::Max, 1, 0, mask(node.a), mask(node.b));
    case ExprOp::Not:
        return instruction(ProgramOp::Sub, 1, 0, constant(1.0f), mask(node.a));
    case ExprOp::Select:
        return instruction(ProgramOp::Select, 1, 0, mask(node.a), mask(node.b), mask(node.c));
    default:
        break;
    }
    const auto operand = [&](const ExprId id) { return graph.isBoolean(id) ? mask(id) : emit(id); };
    switch (node.op) {
    case ExprOp::Less:
        return instruction(ProgramOp::Less, 1, 0, operand(node.a), operand(node.b));
    case ExprOp::LessEqual:
        return instruction(ProgramOp::LessEqual, 1, 0, operand(node.a), operand(node.b));
    case ExprOp::Equal:
        return instruction(ProgramOp::Equal, 1, 0, operand(node.a), operand(node.b));
    case ExprOp::NotEqual:
        return instruction(ProgramOp::NotEqual, 1, 0, operand(node.a), operand(node.b));
    default:
        return fail("A number is used as a condition.");
    }
}

uint32_t ProgramWriter::fail(std::string message) {
    if (!failure)
        failure = std::move(message);
    return constant(0.0f);
}

}
