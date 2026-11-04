#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

std::vector<Scalar> SpirvInterpreter::glsl(const uint32_t type, const GLSLstd450 function, const std::span<const uint32_t> args) {
    const bool integral = types.at(elementType(type)).op == spv::OpTypeInt;
    const auto typed = [&](const ExprId expr) {
        const ExprNode& node = graph.node(expr);
        if (integral && node.op == ExprOp::Constant)
            return integer(static_cast<int64_t>(node.value));
        return integral ? Scalar{expr, std::nullopt} : floating(expr);
    };
    const auto unary = [&](const ExprOp op) { return map1(args[0], [&](const Scalar& x) { return typed(graph.unary(op, x.expr)); }); };
    const auto binary = [&](const ExprOp op) {
        return map2(args[0], args[1], [&](const Scalar& a, const Scalar& b) { return typed(graph.binary(op, a.expr, b.expr)); });
    };
    const auto ternary = [&](auto combine) {
        const std::vector<Scalar>& a = scalars(args[0]);
        const std::vector<Scalar>& b = scalars(args[1]);
        const std::vector<Scalar>& c = scalars(args[2]);
        std::vector<Scalar> result;
        if ((b.size() != 1 && b.size() != a.size()) || (c.size() != 1 && c.size() != a.size())) {
            graph.fail("A ternary function has operands of different sizes.");
            return result;
        }
        for (size_t i = 0; i < a.size(); ++i)
            result.push_back(typed(combine(a[i].expr, b[b.size() == 1 ? 0 : i].expr, c[c.size() == 1 ? 0 : i].expr)));
        return result;
    };
    const auto annotate = [&](std::vector<Scalar> result, const SemanticOp op, const std::vector<Scalar>& a, const std::vector<Scalar>& b,
                              const std::vector<Scalar>& c) {
        std::vector<ExprId> outputs, first, second, third;
        for (const Scalar& value : result)
            outputs.push_back(value.expr);
        for (const Scalar& value : a)
            first.push_back(value.expr);
        for (const Scalar& value : b)
            second.push_back(value.expr);
        for (const Scalar& value : c)
            third.push_back(value.expr);
        graph.annotate(outputs, {.op = op, .a = std::move(first), .b = std::move(second), .c = std::move(third)});
        return result;
    };
    switch (function) {
    case GLSLstd450Round:
    case GLSLstd450RoundEven:
        return unary(ExprOp::Round);
    case GLSLstd450Trunc:
        return unary(ExprOp::Trunc);
    case GLSLstd450FAbs:
    case GLSLstd450SAbs:
        return unary(ExprOp::Abs);
    case GLSLstd450FSign:
    case GLSLstd450SSign:
        return unary(ExprOp::Sign);
    case GLSLstd450Floor:
        return unary(ExprOp::Floor);
    case GLSLstd450Ceil:
        return unary(ExprOp::Ceil);
    case GLSLstd450Fract:
        return unary(ExprOp::Fract);
    case GLSLstd450Sin:
        return unary(ExprOp::Sin);
    case GLSLstd450Cos:
        return unary(ExprOp::Cos);
    case GLSLstd450Tan:
        return unary(ExprOp::Tan);
    case GLSLstd450Asin:
        return unary(ExprOp::Asin);
    case GLSLstd450Acos:
        return unary(ExprOp::Acos);
    case GLSLstd450Atan:
        return unary(ExprOp::Atan);
    case GLSLstd450Atan2:
        return binary(ExprOp::Atan2);
    case GLSLstd450Pow:
        return binary(ExprOp::Pow);
    case GLSLstd450Exp2:
        return unary(ExprOp::Exp2);
    case GLSLstd450Log2:
        return unary(ExprOp::Log2);
    case GLSLstd450Exp:
        return map1(args[0], [&](const Scalar& x) { return floating(graph.unary(ExprOp::Exp2, graph.mul(x.expr, graph.constant(1.4426950408889634f)))); });
    case GLSLstd450Log:
        return map1(args[0], [&](const Scalar& x) { return floating(graph.mul(graph.unary(ExprOp::Log2, x.expr), graph.constant(0.6931471805599453f))); });
    case GLSLstd450Sqrt:
        return unary(ExprOp::Sqrt);
    case GLSLstd450InverseSqrt:
        return map1(args[0], [&](const Scalar& x) { return floating(graph.div(graph.constant(1.0f), graph.unary(ExprOp::Sqrt, x.expr))); });
    case GLSLstd450FMin:
    case GLSLstd450NMin:
    case GLSLstd450UMin:
    case GLSLstd450SMin:
        return binary(ExprOp::Min);
    case GLSLstd450FMax:
    case GLSLstd450NMax:
    case GLSLstd450UMax:
    case GLSLstd450SMax:
        return binary(ExprOp::Max);
    case GLSLstd450FClamp:
    case GLSLstd450NClamp:
    case GLSLstd450UClamp:
    case GLSLstd450SClamp: {
        const auto result = ternary([&](ExprId x, ExprId low, ExprId high) { return graph.clamp(x, low, high); });
        return annotate(result, SemanticOp::Clamp, scalars(args[0]), scalars(args[1]), scalars(args[2]));
    }
    case GLSLstd450FMix: {
        const auto result = ternary([&](ExprId a, ExprId b, ExprId t) { return graph.add(a, graph.mul(graph.sub(b, a), t)); });
        return annotate(result, SemanticOp::Mix, scalars(args[0]), scalars(args[1]), scalars(args[2]));
    }
    case GLSLstd450Fma:
        return ternary([&](ExprId a, ExprId b, ExprId c) { return graph.add(graph.mul(a, b), c); });
    case GLSLstd450Step:
        return map2(args[0], args[1], [&](const Scalar& edge, const Scalar& x) {
            return floating(graph.select(graph.less(x.expr, edge.expr), graph.constant(0.0f), graph.constant(1.0f)));
        });
    case GLSLstd450SmoothStep: {
        const auto result = ternary([&](ExprId low, ExprId high, ExprId x) {
            const ExprId t = graph.clamp(graph.div(graph.sub(x, low), graph.sub(high, low)), graph.constant(0.0f), graph.constant(1.0f));
            return graph.mul(graph.mul(t, t), graph.sub(graph.constant(3.0f), graph.mul(graph.constant(2.0f), t)));
        });
        const std::vector<Scalar>& low = scalars(args[0]);
        const std::vector<Scalar>& high = scalars(args[1]);
        if (graph.isFailed())
            return result;
        return annotate(result, SemanticOp::Smoothstep, scalars(args[2]), low, high);
    }
    case GLSLstd450Length: {
        ExprId sum = zero();
        for (const Scalar& x : scalars(args[0]))
            sum = graph.add(sum, graph.mul(x.expr, x.expr));
        return annotate({floating(graph.unary(ExprOp::Sqrt, sum))}, SemanticOp::Length, scalars(args[0]), {}, {});
    }
    case GLSLstd450Distance: {
        const std::vector<Scalar>& a = scalars(args[0]);
        const std::vector<Scalar>& b = scalars(args[1]);
        if (a.size() != b.size()) {
            graph.fail("A distance has vectors of different sizes.");
            return {};
        }
        ExprId sum = zero();
        for (size_t i = 0; i < a.size(); ++i) {
            const ExprId d = graph.sub(a[i].expr, b[i].expr);
            sum = graph.add(sum, graph.mul(d, d));
        }
        return annotate({floating(graph.unary(ExprOp::Sqrt, sum))}, SemanticOp::Distance, scalars(args[0]), scalars(args[1]), {});
    }
    case GLSLstd450Normalize: {
        const std::vector<Scalar>& x = scalars(args[0]);
        ExprId sum = zero();
        for (const Scalar& c : x)
            sum = graph.add(sum, graph.mul(c.expr, c.expr));
        const ExprId inverse = graph.div(graph.constant(1.0f), graph.unary(ExprOp::Sqrt, sum));
        std::vector<Scalar> result;
        result.reserve(x.size());
        for (const Scalar& c : x)
            result.push_back(floating(graph.mul(c.expr, inverse)));
        return annotate(std::move(result), SemanticOp::Normalize, x, {}, {});
    }
    case GLSLstd450Cross: {
        const std::vector<Scalar>& a = scalars(args[0]);
        const std::vector<Scalar>& b = scalars(args[1]);
        if (a.size() != 3 || b.size() != 3) {
            graph.fail("A cross product has operands that are not three-component vectors.");
            return {};
        }
        const auto term = [&](int i, int j) { return graph.sub(graph.mul(a[i].expr, b[j].expr), graph.mul(a[j].expr, b[i].expr)); };
        return annotate({floating(term(1, 2)), floating(term(2, 0)), floating(term(0, 1))}, SemanticOp::Cross, a, b, {});
    }
    case GLSLstd450Reflect: {
        const std::vector<Scalar>& i = scalars(args[0]);
        const std::vector<Scalar>& n = scalars(args[1]);
        if (i.size() != n.size()) {
            graph.fail("A reflect has vectors of different sizes.");
            return {};
        }
        ExprId d = zero();
        for (size_t k = 0; k < i.size(); ++k)
            d = graph.add(d, graph.mul(i[k].expr, n[k].expr));
        std::vector<Scalar> result;
        result.reserve(i.size());
        for (size_t k = 0; k < i.size(); ++k)
            result.push_back(floating(graph.sub(i[k].expr, graph.mul(graph.mul(graph.constant(2.0f), d), n[k].expr))));
        if (i.size() == 3)
            return annotate(std::move(result), SemanticOp::Reflect, i, n, {});
        return result;
    }
    default:
        (void)type;
        graph.fail("The shader uses the unsupported GLSL.std.450 function " + std::to_string(function) + ".");
        return {};
    }
}

}
