#pragma once

#include <span>

#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

enum class EPreshaderOpcode : uint8 {
    Nop,
    ConstantZero,
    Constant,
    Parameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
    Modulo,
    Min,
    Max,
    Clamp,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Atan2,
    Dot,
    Cross,
    Sqrt,
    Rcp,
    Length,
    Normalize,
    Saturate,
    Abs,
    Floor,
    Ceil,
    Round,
    Trunc,
    Sign,
    Frac,
    Fractional,
    Log2,
    Log10,
    ComponentSwizzle,
    AppendVector,
    TextureSize,
    TexelSize,
    ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset,
    RuntimeVirtualTextureUniform,
    SparseVolumeTextureUniform,
    GetField,
    SetField,
    Neg,
    Jump,
    JumpIfFalse,
    PushValue,
    Less,
    Assign,
    Greater,
    LessEqual,
    GreaterEqual,
    Exp,
    Exp2,
    Log,
};

struct FPreshaderConstant {
    uint8 NumComponents = 0u;
    double Components[4] = {};
};

struct FPreshaderSwizzle {
    uint8 NumComponents = 0u;
    uint8 ComponentIndices[4] = {};
};

struct FPreshaderTextureParameter {
    uint16 NameIndex = 0u;
    int32 ParameterIndex = INDEX_NONE;
    uint8 Association = 0u;
    int32 TextureIndex = INDEX_NONE;
};

uint32 GetPreshaderValueComponents(uint8 ValueType);

class FPreshaderReader {
public:
    FPreshaderReader(std::span<const uint8> InProgram, EGame InGame);

    bool IsAtEnd() const { return Offset >= Program.size(); }
    uint8 GetRawOpcode() const { return RawOpcode; }

    Result<EPreshaderOpcode> ReadOpcode();
    Result<uint32> ReadZeroComponents();
    Result<FPreshaderConstant> ReadConstant();
    Result<uint16> ReadParameterIndex();
    Result<FPreshaderSwizzle> ReadSwizzle();
    Result<FPreshaderTextureParameter> ReadTextureParameter();

private:
    template <class T>
    Result<T> Read();

    std::span<const uint8> Program;
    std::span<const EPreshaderOpcode> Opcodes;
    EGame Game;
    std::size_t Offset = 0;
    uint8 RawOpcode = 0u;
};

}
