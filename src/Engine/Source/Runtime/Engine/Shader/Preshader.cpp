#include "Engine/Source/Runtime/Engine/Shader/Preshader.h"

#include <cstring>

namespace ue5parse::runtime {

namespace {

using enum EPreshaderOpcode;

constexpr EPreshaderOpcode Opcodes500[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Min,  Max,  Clamp, Sin,  Cos,   Tan,        Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform,
};

constexpr EPreshaderOpcode Opcodes501[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Min,  Max,  Clamp, Sin,  Cos,   Tan,        Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform, GetField, SetField, Neg, Jump, JumpIfFalse, PushValue, Less, Assign,
    Greater, LessEqual, GreaterEqual,
};

constexpr EPreshaderOpcode Opcodes502[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Min,  Max,  Clamp, Sin,  Cos,   Tan,        Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform, SparseVolumeTextureUniform, GetField, SetField, Neg, Jump, JumpIfFalse,
    PushValue, Less, Assign, Greater, LessEqual, GreaterEqual,
};

constexpr EPreshaderOpcode Opcodes503[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Min,  Max,  Clamp, Sin,  Cos,   Tan,        Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform, SparseVolumeTextureUniform, GetField, SetField, Neg, Jump, JumpIfFalse,
    PushValue, Less, Assign, Greater, LessEqual, GreaterEqual, Exp, Exp2, Log,
};

constexpr EPreshaderOpcode Opcodes505[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Modulo, Min, Max,  Clamp, Sin,  Cos,   Tan,  Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform, SparseVolumeTextureUniform, GetField, SetField, Neg, Jump, JumpIfFalse,
    PushValue, Less, Assign, Greater, LessEqual, GreaterEqual, Exp, Exp2, Log,
};

constexpr EPreshaderOpcode Opcodes508[] = {
    Nop,  ConstantZero, Constant, Parameter, Add,   Sub,    Mul,   Div,   Fmod,  Modulo, Min, Max,  Clamp, Sin,  Cos,   Tan,  Asin,
    Acos, Atan,         Atan2,    Dot,       Cross, Sqrt,   Rcp,   Length, Normalize, Saturate, Abs, Floor, Ceil, Round, Trunc, Sign,
    Frac, Fractional,   Log2,     Log10,     ComponentSwizzle, AppendVector, TextureSize, TexelSize, ExternalTextureCoordinateScaleRotation,
    ExternalTextureCoordinateOffset, RuntimeVirtualTextureUniform, SparseVolumeTextureUniform, Neg, PushValue, Less, Assign, Greater,
    LessEqual, GreaterEqual, Exp, Exp2, Log,
};

std::span<const EPreshaderOpcode> GetOpcodes(EGame Game) {
    if (Game < EGame::GAME_UE5_1)
        return Opcodes500;
    if (Game < EGame::GAME_UE5_2)
        return Opcodes501;
    if (Game < EGame::GAME_UE5_3)
        return Opcodes502;
    if (Game < EGame::GAME_UE5_5)
        return Opcodes503;
    if (Game < EGame::GAME_UE5_8)
        return Opcodes505;
    return Opcodes508;
}

}

uint32 GetPreshaderValueComponents(uint8 ValueType) {
    if (ValueType == 0u || ValueType > 20u)
        return 0u;
    return (ValueType - 1u) % 4u + 1u;
}

FPreshaderReader::FPreshaderReader(std::span<const uint8> InProgram, EGame InGame) : Program(InProgram), Opcodes(GetOpcodes(InGame)), Game(InGame) {}

template <class T>
Result<T> FPreshaderReader::Read() {
    if (sizeof(T) > Program.size() - Offset)
        return fail(ErrorCode::ReadPastEnd, "FPreshaderReader.Read", Offset);
    T Value;
    std::memcpy(&Value, Program.data() + Offset, sizeof(T));
    Offset += sizeof(T);
    return Value;
}

Result<EPreshaderOpcode> FPreshaderReader::ReadOpcode() {
    Result<uint8> Raw = Read<uint8>();
    if (!Raw)
        return std::unexpected(Raw.error());
    RawOpcode = *Raw;
    if (RawOpcode >= Opcodes.size())
        return fail(ErrorCode::InvalidData, "FPreshaderReader.ReadOpcode", Offset - 1);
    return Opcodes[RawOpcode];
}

Result<uint32> FPreshaderReader::ReadZeroComponents() {
    if (Game < EGame::GAME_UE5_1)
        return 1u;
    Result<uint8> ValueType = Read<uint8>();
    if (!ValueType)
        return std::unexpected(ValueType.error());
    uint32 Components = GetPreshaderValueComponents(*ValueType);
    if (Components == 0u)
        return fail(ErrorCode::InvalidData, "FPreshaderReader.ReadZeroComponents", Offset - 1);
    return Components;
}

Result<FPreshaderConstant> FPreshaderReader::ReadConstant() {
    Result<uint8> ValueType = Read<uint8>();
    if (!ValueType)
        return std::unexpected(ValueType.error());
    FPreshaderConstant Constant;
    Constant.NumComponents = static_cast<uint8>(GetPreshaderValueComponents(*ValueType));
    if (Constant.NumComponents == 0u)
        return fail(ErrorCode::InvalidData, "FPreshaderReader.ReadConstant", Offset - 1);
    for (uint8 Index = 0u; Index < Constant.NumComponents; ++Index) {
        Result<double> Component = *ValueType <= 4u                            ? Read<float>().transform([](float V) { return double(V); })
                                   : *ValueType <= 8u || *ValueType >= 17u     ? Read<double>()
                                   : *ValueType <= 12u                         ? Read<int32>().transform([](int32 V) { return double(V); })
                                                                               : Read<uint8>().transform([](uint8 V) { return V != 0u ? 1.0 : 0.0; });
        if (!Component)
            return std::unexpected(Component.error());
        Constant.Components[Index] = *Component;
    }
    return Constant;
}

Result<uint16> FPreshaderReader::ReadParameterIndex() { return Read<uint16>(); }

Result<FPreshaderSwizzle> FPreshaderReader::ReadSwizzle() {
    FPreshaderSwizzle Swizzle;
    Result<uint8> Count = Read<uint8>();
    if (!Count)
        return std::unexpected(Count.error());
    Swizzle.NumComponents = *Count;
    for (uint8& ComponentIndex : Swizzle.ComponentIndices) {
        Result<uint8> Index = Read<uint8>();
        if (!Index)
            return std::unexpected(Index.error());
        ComponentIndex = *Index;
    }
    return Swizzle;
}

Result<FPreshaderTextureParameter> FPreshaderReader::ReadTextureParameter() {
    Result<uint16> NameIndex = Read<uint16>();
    Result<int32> ParameterIndex = NameIndex.and_then([&](uint16) { return Read<int32>(); });
    Result<uint8> Association = ParameterIndex.and_then([&](int32) { return Read<uint8>(); });
    Result<int32> TextureIndex = Association.and_then([&](uint8) { return Read<int32>(); });
    if (!TextureIndex)
        return std::unexpected(TextureIndex.error());
    return FPreshaderTextureParameter{*NameIndex, *ParameterIndex, *Association, *TextureIndex};
}

}
