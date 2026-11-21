#include "Engine/Source/Runtime/Engine/Shader/Preshader.h"

#include <algorithm>
#include <cstring>

namespace ue5parse::runtime {

namespace {

using enum EPreshaderOpcode;

constexpr EPreshaderOpcode Opcodes425[] = {
    Nop,
    ConstantZero,
    Constant,
    ScalarParameter,
    VectorParameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
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
    Length,
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
};

constexpr EPreshaderOpcode Opcodes500[] = {
    Nop,
    ConstantZero,
    Constant,
    Parameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
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
};

constexpr EPreshaderOpcode Opcodes501[] = {
    Nop,
    ConstantZero,
    Constant,
    Parameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
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
};

constexpr EPreshaderOpcode Opcodes502[] = {
    Nop,
    ConstantZero,
    Constant,
    Parameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
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
};

constexpr EPreshaderOpcode Opcodes503[] = {
    Nop,
    ConstantZero,
    Constant,
    Parameter,
    Add,
    Sub,
    Mul,
    Div,
    Fmod,
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

constexpr EPreshaderOpcode Opcodes505[] = {
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

constexpr EPreshaderOpcode Opcodes508[] = {
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
    Neg,
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

std::span<const EPreshaderOpcode> GetOpcodes(EGame Game) {
    if (Game < EGame::GAME_UE5_0)
        return Opcodes425;
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

std::optional<uint8> FindPreshaderOpcode(EGame Game, EPreshaderOpcode Opcode) {
    std::span<const EPreshaderOpcode> Opcodes = GetOpcodes(Game);
    auto Found = std::ranges::find(Opcodes, Opcode);
    if (Found == Opcodes.end())
        return std::nullopt;
    return static_cast<uint8>(Found - Opcodes.begin());
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
    if (Game < EGame::GAME_UE5_0)
        return ReadLinearColorConstant();
    Result<uint8> ValueType = Read<uint8>();
    if (!ValueType)
        return std::unexpected(ValueType.error());
    FPreshaderConstant Constant;
    Constant.NumComponents = static_cast<uint8>(GetPreshaderValueComponents(*ValueType));
    if (Constant.NumComponents == 0u)
        return fail(ErrorCode::InvalidData, "FPreshaderReader.ReadConstant", Offset - 1);
    for (uint8 Index = 0u; Index < Constant.NumComponents; ++Index) {
        Result<double> Component = *ValueType <= 4u                        ? Read<float>().transform([](float V) { return double(V); })
                                   : *ValueType <= 8u || *ValueType >= 17u ? Read<double>()
                                   : *ValueType <= 12u                     ? Read<int32>().transform([](int32 V) { return double(V); })
                                                                           : Read<uint8>().transform([](uint8 V) { return V != 0u ? 1.0 : 0.0; });
        if (!Component)
            return std::unexpected(Component.error());
        Constant.Components[Index] = *Component;
    }
    return Constant;
}

//! UE4 writes every constant as an FLinearColor.
Result<FPreshaderConstant> FPreshaderReader::ReadLinearColorConstant() {
    FPreshaderConstant Constant;
    Constant.NumComponents = 4u;
    for (double& Component : Constant.Components) {
        Result<float> Value = Read<float>();
        if (!Value)
            return std::unexpected(Value.error());
        Component = *Value;
    }
    return Constant;
}

Result<uint16> FPreshaderReader::ReadParameterIndex() {
    return Read<uint16>();
}

//! UE4 writes one operand byte after Dot and Cross, their EMaterialValueType, and after AppendVector, the component count of its first operand.
Result<std::optional<uint8>> FPreshaderReader::ReadLegacyOperand() {
    if (Game >= EGame::GAME_UE5_0)
        return std::optional<uint8>();
    return Read<uint8>().transform([](uint8 Operand) { return std::optional<uint8>(Operand); });
}

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

//! UE 4.25 writes the FHashedMaterialParameterInfo as its 16 bytes in memory: the 8-byte name hash, the int32 index, the byte association and 3 bytes of padding.
Result<FPreshaderTextureParameter> FPreshaderReader::ReadTextureParameter() {
    if (Game < EGame::GAME_UE4_26) {
        Result<uint64> NameHash = Read<uint64>();
        Result<int32> ParameterIndex = NameHash.and_then([&](uint64) { return Read<int32>(); });
        Result<uint32> Association = ParameterIndex.and_then([&](int32) { return Read<uint32>(); });
        Result<int32> TextureIndex = Association.and_then([&](uint32) { return Read<int32>(); });
        if (!TextureIndex)
            return std::unexpected(TextureIndex.error());
        return FPreshaderTextureParameter{0u, *NameHash, *ParameterIndex, static_cast<uint8>(*Association & 0xffu), *TextureIndex};
    }
    Result<uint16> NameIndex = Read<uint16>();
    Result<int32> ParameterIndex = NameIndex.and_then([&](uint16) { return Read<int32>(); });
    Result<uint8> Association = ParameterIndex.and_then([&](int32) { return Read<uint8>(); });
    Result<int32> TextureIndex = Association.and_then([&](uint8) { return Read<int32>(); });
    if (!TextureIndex)
        return std::unexpected(TextureIndex.error());
    return FPreshaderTextureParameter{*NameIndex, 0u, *ParameterIndex, *Association, *TextureIndex};
}

}
