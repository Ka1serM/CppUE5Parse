#pragma once

#include <cstdint>
#include <vector>

namespace ue5parse::conversion {

enum class ProgramOp : uint8_t {
    Constant,
    Parameter,
    Texture,
    Geometry,
    GeometryDx,
    GeometryDy,
    Time,
    Frame,
    Extract,
    Combine,
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
    Select,
    Dot,
    Cross,
    Length,
    Distance,
    Normalize,
    Reflect,
    TangentToWorld,
    Clamp,
    Mix,
    Smoothstep,
    MulAdd,
};

struct ProgramInstruction {
    ProgramOp op = ProgramOp::Constant;
    uint8_t width = 0;
    uint16_t index = 0;
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
};

struct ExpressionProgram {
    std::vector<ProgramInstruction> instructions;
    std::vector<float> constants;
    std::vector<uint32_t> outputs;
};

}
