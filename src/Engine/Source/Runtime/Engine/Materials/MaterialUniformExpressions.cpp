#include "Engine/Source/Runtime/Engine/Materials/MaterialUniformExpressions.h"

#include <cstring>
#include <optional>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Shader/Preshader.h"

namespace ue5parse::runtime {

namespace {

constexpr uint64 ExpressionType(std::string_view Name) {
    return FName(Name).getHash();
}

constexpr uint8 ValueTypeFloat1 = 1u;
constexpr uint8 ValueTypeFloat4 = 4u;
constexpr uint8 MaterialValueTypeFloat2 = 2u;
constexpr uint8 MaterialValueTypeFloat3 = 4u;
constexpr uint8 MaterialValueTypeFloat4 = 8u;
constexpr uint32 MaterialValueTypeFloat = 15u;

enum EFoldedMathOperation : uint8 { FMO_Add, FMO_Sub, FMO_Mul, FMO_Div, FMO_Dot, FMO_Cross };
enum ETrigMathOperation : uint8 { TMO_Sin, TMO_Cos, TMO_Tan, TMO_Asin, TMO_Acos, TMO_Atan, TMO_Atan2 };
enum ETextureProperty : int8 { TMTM_TextureSize, TMTM_TexelSize };

struct FLegacyExpression {
    std::optional<FMaterialTextureParameterInfo> Texture;
    std::optional<FMaterialExternalTextureParameterInfo> ExternalTexture;
};

//! Before UE 4.25 a material keeps its uniform expressions as trees; the reader writes each tree as the UE 4.25 preshader program that WriteNumberOpcodes makes of it.
class FLegacyUniformExpressionReader {
public:
    FLegacyUniformExpressionReader(FArchive& InAr, FUniformExpressionSet& InValue) : Ar(InAr), Value(InValue), Code(&InValue.UniformPreshaderData.Data) {}

    void ReadNumberExpressions(TArray<FMaterialUniformPreshaderHeader>& OutPreshaders) {
        int32 NumExpressions = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumExpressions && !Ar.IsError(); Index++) {
            FMaterialUniformPreshaderHeader& Header = OutPreshaders.Emplace_GetRef();
            Header.OpcodeOffset = static_cast<uint32>(Code->Num());
            WriteNumber(ReadExpression());
            Header.OpcodeSize = static_cast<uint32>(Code->Num()) - Header.OpcodeOffset;
        }
    }

    void ReadTextureExpressions(EMaterialTextureParameterType Type) {
        int32 NumExpressions = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumExpressions && !Ar.IsError(); Index++) {
            FLegacyExpression Expression = ReadExpression();
            if (!Expression.Texture) {
                Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.Texture");
                return;
            }
            Value.UniformTextureParameters[static_cast<uint32>(Type)].Add(*Expression.Texture);
        }
    }

    void ReadExternalTextureExpressions() {
        int32 NumExpressions = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumExpressions && !Ar.IsError(); Index++) {
            FLegacyExpression Expression = ReadExpression();
            if (!Expression.ExternalTexture) {
                Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.ExternalTexture");
                return;
            }
            Value.UniformExternalTextureParameters.Add(*Expression.ExternalTexture);
        }
    }

    //! Before UE 4.20 the expressions that change every frame go to loose shader parameters instead of the material uniform buffer.
    void ReadPerFrameExpressions() {
        TArray<uint8> PerFrameCode;
        TArray<uint8>* MaterialCode = Code;
        Code = &PerFrameCode;
        bPerFrame = true;
        int32 NumExpressions = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumExpressions && !Ar.IsError(); Index++)
            WriteNumber(ReadExpression());
        bPerFrame = false;
        Code = MaterialCode;
    }

    void ReadVirtualTextureStacks() {
        int32 NumStacks = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumStacks && !Ar.IsError(); Index++) {
            FMaterialVirtualTextureStack& Stack = Value.VTStacks.Emplace_GetRef();
            uint32 NumLayers = Ar.Read<uint32>();
            Stack.NumLayers = std::min<uint32>(NumLayers, std::size(Stack.LayerUniformExpressionIndices));
            for (int32& LayerIndex : Stack.LayerUniformExpressionIndices)
                LayerIndex = INDEX_NONE;
            for (uint32 Layer = 0u; Layer < Stack.NumLayers; Layer++)
                Stack.LayerUniformExpressionIndices[Layer] = Ar.Read<int32>();
            Ar.SkipBytes(static_cast<int64>(NumLayers - Stack.NumLayers) * static_cast<int64>(sizeof(int32)));
            Stack.PreallocatedStackTextureIndex = Ar.Read<int32>();
        }
    }

    //! The preshader reads the scalar parameters first and the vector parameters after them.
    void AddNumericParameters() {
        for (const FNumericParameter& Parameter : ScalarParameters)
            AddNumericParameter(Parameter, EMaterialParameterType::Scalar, sizeof(float));
        for (const FNumericParameter& Parameter : VectorParameters)
            AddNumericParameter(Parameter, EMaterialParameterType::Vector, sizeof(FLinearColor));
    }

private:
    static constexpr int32 MaxDepth = 256;

    struct FNumericParameter {
        FHashedMaterialParameterInfo ParameterInfo;
        FLinearColor DefaultValue;
    };

    FLegacyExpression ReadExpression() {
        if (Depth >= MaxDepth) {
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.Depth");
            return {};
        }
        Depth++;
        FName TypeName;
        Ar << TypeName;
        FLegacyExpression Expression = Ar.IsError() ? FLegacyExpression() : ReadExpressionData(TypeName);
        Depth--;
        return Expression;
    }

    //! UE 4.25 writes ConstantZero for an expression that has no number value, such as a texture.
    void WriteNumber(const FLegacyExpression& Expression) {
        if (Expression.Texture || Expression.ExternalTexture)
            WriteOpcode(EPreshaderOpcode::ConstantZero);
    }

    void ReadNumber() { WriteNumber(ReadExpression()); }

    void SkipNumber() {
        const int32 Offset = Code->Num();
        ReadNumber();
        Code->SetNum(Offset);
    }

    FLegacyExpression ReadExpressionData(FName TypeName) {
        switch (TypeName.getHash()) {
        case ExpressionType("FMaterialUniformExpressionAbs"):
            return ReadUnary(EPreshaderOpcode::Abs);
        case ExpressionType("FMaterialUniformExpressionCeil"):
            return ReadUnary(EPreshaderOpcode::Ceil);
        case ExpressionType("FMaterialUniformExpressionFloor"):
            return ReadUnary(EPreshaderOpcode::Floor);
        case ExpressionType("FMaterialUniformExpressionFrac"):
            return ReadUnary(EPreshaderOpcode::Frac);
        case ExpressionType("FMaterialUniformExpressionPeriodic"):
            return ReadUnary(EPreshaderOpcode::Fractional);
        case ExpressionType("FMaterialUniformExpressionSquareRoot"):
            return ReadUnary(EPreshaderOpcode::Sqrt);
        case ExpressionType("FMaterialUniformExpressionRound"):
            return ReadUnary(EPreshaderOpcode::Round);
        case ExpressionType("FMaterialUniformExpressionTruncate"):
            return ReadUnary(EPreshaderOpcode::Trunc);
        case ExpressionType("FMaterialUniformExpressionSign"):
            return ReadUnary(EPreshaderOpcode::Sign);
        case ExpressionType("FMaterialUniformExpressionLogarithm2"):
            return ReadUnary(EPreshaderOpcode::Log2);
        case ExpressionType("FMaterialUniformExpressionLogarithm10"):
            return ReadUnary(EPreshaderOpcode::Log10);
        case ExpressionType("FMaterialUniformExpressionSaturate"):
            return ReadUnary(EPreshaderOpcode::Saturate);
        case ExpressionType("FMaterialUniformExpressionLength"):
            ReadLength();
            return {};
        case ExpressionType("FMaterialUniformExpressionMin"):
            return ReadBinary(EPreshaderOpcode::Min);
        case ExpressionType("FMaterialUniformExpressionMax"):
            return ReadBinary(EPreshaderOpcode::Max);
        case ExpressionType("FMaterialUniformExpressionFmod"):
            return ReadBinary(EPreshaderOpcode::Fmod);
        case ExpressionType("FMaterialUniformExpressionAppendVector"):
            ReadNumber();
            ReadNumber();
            WriteOpcode(EPreshaderOpcode::AppendVector);
            Write(static_cast<uint8>(Ar.Read<uint32>()));
            return {};
        case ExpressionType("FMaterialUniformExpressionClamp"):
            ReadNumber();
            ReadNumber();
            ReadNumber();
            WriteOpcode(EPreshaderOpcode::Clamp);
            return {};
        case ExpressionType("FMaterialUniformExpressionComponentSwizzle"):
            ReadComponentSwizzle();
            return {};
        case ExpressionType("FMaterialUniformExpressionConstant"): {
            FLinearColor Constant;
            Ar << Constant;
            Ar.Read<uint8>();
            WriteOpcode(EPreshaderOpcode::Constant);
            Write(Constant);
            return {};
        }
        case ExpressionType("FMaterialUniformExpressionFoldedMath"):
            ReadFoldedMath();
            return {};
        case ExpressionType("FMaterialUniformExpressionSine"): {
            ReadNumber();
            bool bIsCosine = false;
            Ar << bIsCosine;
            WriteOpcode(bIsCosine ? EPreshaderOpcode::Cos : EPreshaderOpcode::Sin);
            return {};
        }
        case ExpressionType("FMaterialUniformExpressionTrigMath"):
            ReadTrigMath();
            return {};
        case ExpressionType("FMaterialUniformExpressionTime"):
        case ExpressionType("FMaterialUniformExpressionRealTime"):
            if (!bPerFrame)
                Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpressionTime.PerFrame");
            return {};
        case ExpressionType("FMaterialUniformExpressionScalarParameter"): {
            FNumericParameter Parameter{ReadParameterName(), {}};
            Parameter.DefaultValue.R = Ar.Read<float>();
            WriteOpcode(EPreshaderOpcode::ScalarParameter);
            Write(FindOrAddParameter(ScalarParameters, Parameter));
            return {};
        }
        case ExpressionType("FMaterialUniformExpressionVectorParameter"): {
            FNumericParameter Parameter{ReadParameterName(), {}};
            Ar << Parameter.DefaultValue;
            WriteOpcode(EPreshaderOpcode::VectorParameter);
            Write(FindOrAddParameter(VectorParameters, Parameter));
            return {};
        }
        case ExpressionType("FMaterialUniformExpressionTexture"):
        case ExpressionType("FMaterialUniformExpressionFlipBookTextureParameter"):
            return {ReadTexture({}), std::nullopt};
        case ExpressionType("FMaterialUniformExpressionTextureParameter"): {
            const FHashedMaterialParameterInfo ParameterInfo = ReadParameterName();
            return {ReadTexture(ParameterInfo), std::nullopt};
        }
        case ExpressionType("FMaterialUniformExpressionTextureProperty"):
            ReadTextureProperty();
            return {};
        case ExpressionType("FMaterialUniformExpressionExternalTexture"):
            return {std::nullopt, ReadExternalTexture({})};
        case ExpressionType("FMaterialUniformExpressionExternalTextureParameter"): {
            FName ParameterName;
            Ar << ParameterName;
            return {std::nullopt, ReadExternalTexture(ParameterName)};
        }
        case ExpressionType("FMaterialUniformExpressionExternalTextureCoordinateScaleRotation"):
            ReadExternalTextureCoordinate(EPreshaderOpcode::ExternalTextureCoordinateScaleRotation);
            return {};
        case ExpressionType("FMaterialUniformExpressionExternalTextureCoordinateOffset"):
            ReadExternalTextureCoordinate(EPreshaderOpcode::ExternalTextureCoordinateOffset);
            return {};
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureParameter"):
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureParameter_DEPRECATED"):
            WriteRuntimeVirtualTextureUniform({});
            return {};
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureUniform"): {
            bool bParameter = false;
            Ar << bParameter;
            const FHashedMaterialParameterInfo ParameterInfo = ReadParameterInfo();
            WriteRuntimeVirtualTextureUniform(bParameter ? ParameterInfo : FHashedMaterialParameterInfo());
            return {};
        }
        default:
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.Type");
            return {};
        }
    }

    FLegacyExpression ReadUnary(EPreshaderOpcode Opcode) {
        ReadNumber();
        WriteOpcode(Opcode);
        return {};
    }

    FLegacyExpression ReadBinary(EPreshaderOpcode Opcode) {
        ReadNumber();
        ReadNumber();
        WriteOpcode(Opcode);
        return {};
    }

    //! Before UE 4.14 Length and Dot keep no value type: Length takes three components and Dot four.
    std::optional<uint32> ReadValueType() {
        if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TypeHandlingForMaterialSqrtNodes, EGame::GAME_UE4_14))
            return std::nullopt;
        return Ar.Read<uint32>();
    }

    static uint8 GetValueTypeComponents(uint32 ValueType) {
        return static_cast<uint8>(1u + (ValueType >= MaterialValueTypeFloat2) + (ValueType >= MaterialValueTypeFloat3) + (ValueType >= MaterialValueTypeFloat4));
    }

    //! The Length opcode takes the length of every component, so the reader first keeps the components the UE4 Length reads.
    void ReadLength() {
        ReadNumber();
        const std::optional<uint32> ValueType = ReadValueType();
        WriteOpcode(EPreshaderOpcode::ComponentSwizzle);
        Write(ValueType ? GetValueTypeComponents(*ValueType) : uint8(3u));
        for (uint8 Component = 0u; Component < 4u; Component++)
            Write(Component);
        WriteOpcode(EPreshaderOpcode::Length);
    }

    void ReadFoldedMath() {
        ReadNumber();
        ReadNumber();
        const uint8 Operation = Ar.Read<uint8>();
        const uint32 ValueType = ReadValueType().value_or(MaterialValueTypeFloat);
        switch (Operation) {
        case FMO_Add:
            WriteOpcode(EPreshaderOpcode::Add);
            return;
        case FMO_Sub:
            WriteOpcode(EPreshaderOpcode::Sub);
            return;
        case FMO_Mul:
            WriteOpcode(EPreshaderOpcode::Mul);
            return;
        case FMO_Div:
            WriteOpcode(EPreshaderOpcode::Div);
            return;
        case FMO_Dot:
            WriteOpcode(EPreshaderOpcode::Dot);
            Write(static_cast<uint8>(ValueType));
            return;
        case FMO_Cross:
            WriteOpcode(EPreshaderOpcode::Cross);
            Write(static_cast<uint8>(ValueType));
            return;
        default:
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpressionFoldedMath.Op");
            return;
        }
    }

    //! The unary operations keep their operand as Y too, and only Atan2 reads Y.
    void ReadTrigMath() {
        ReadNumber();
        const int32 Offset = Code->Num();
        ReadNumber();
        const uint8 Operation = Ar.Read<uint8>();
        if (Operation != TMO_Atan2)
            Code->SetNum(Offset);
        constexpr EPreshaderOpcode Opcodes[] = {EPreshaderOpcode::Sin,  EPreshaderOpcode::Cos,  EPreshaderOpcode::Tan,  EPreshaderOpcode::Asin,
                                                EPreshaderOpcode::Acos, EPreshaderOpcode::Atan, EPreshaderOpcode::Atan2};
        if (Operation >= std::size(Opcodes)) {
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpressionTrigMath.Op");
            return;
        }
        WriteOpcode(Opcodes[Operation]);
    }

    void ReadComponentSwizzle() {
        ReadNumber();
        uint8 Indices[4];
        for (uint8& Index : Indices)
            Index = Ar.Read<uint8>();
        const uint8 NumElements = Ar.Read<uint8>();
        WriteOpcode(EPreshaderOpcode::ComponentSwizzle);
        Write(NumElements);
        for (uint8 Index : Indices)
            Write(Index);
    }

    void ReadTextureProperty() {
        const FLegacyExpression Expression = ReadExpression();
        const int8 Property = Ar.Read<int8>();
        if (!Expression.Texture) {
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpressionTextureProperty.TextureExpression");
            return;
        }
        if (Property != TMTM_TextureSize && Property != TMTM_TexelSize) {
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpressionTextureProperty.TextureProperty");
            return;
        }
        WriteOpcode(Property == TMTM_TextureSize ? EPreshaderOpcode::TextureSize : EPreshaderOpcode::TexelSize);
        Write(Expression.Texture->ParameterInfo);
        Write(Expression.Texture->TextureIndex);
    }

    void ReadExternalTextureCoordinate(EPreshaderOpcode Opcode) {
        bool bHasParameterName = false;
        Ar << bHasParameterName;
        FName ParameterName;
        if (bHasParameterName)
            Ar << ParameterName;
        const FMaterialExternalTextureParameterInfo ExternalTexture = ReadExternalTexture(ParameterName);
        WriteOpcode(Opcode);
        Write(bHasParameterName ? FHashedName(ParameterName).GetHash() : uint64(0u));
        Write(ExternalTexture.ExternalTextureGuid);
        Write(ExternalTexture.SourceTextureIndex);
    }

    void WriteRuntimeVirtualTextureUniform(const FHashedMaterialParameterInfo& ParameterInfo) {
        const int32 TextureIndex = Ar.Read<int32>();
        const int32 VectorIndex = Ar.Read<int32>();
        WriteOpcode(EPreshaderOpcode::RuntimeVirtualTextureUniform);
        Write(ParameterInfo);
        Write(TextureIndex);
        Write(VectorIndex);
    }

    FHashedMaterialParameterInfo ReadParameterInfo() {
        FHashedMaterialParameterInfo ParameterInfo;
        Ar << ParameterInfo.Name;
        ParameterInfo.Association = static_cast<EMaterialParameterAssociation>(Ar.Read<uint8>());
        ParameterInfo.Index = Ar.Read<int32>();
        return ParameterInfo;
    }

    FHashedMaterialParameterInfo ReadParameterName() {
        if (Ar.GetGame() >= EGame::GAME_UE4_19)
            return ReadParameterInfo();
        FHashedMaterialParameterInfo ParameterInfo;
        Ar << ParameterInfo.Name;
        return ParameterInfo;
    }

    FMaterialTextureParameterInfo ReadTexture(const FHashedMaterialParameterInfo& ParameterInfo) {
        FMaterialTextureParameterInfo Texture;
        Texture.ParameterInfo = ParameterInfo;
        Texture.TextureIndex = Ar.Read<int32>();
        if (Ar.GetGame() >= EGame::GAME_UE4_24) {
            Texture.VirtualTextureLayerIndex = static_cast<uint8>(Ar.Read<int16>());
            Ar.Read<int16>();
        } else if (Ar.GetGame() >= EGame::GAME_UE4_23) {
            Texture.VirtualTextureLayerIndex = static_cast<uint8>(Ar.Read<int32>());
        }
        if (Ar.GetGame() >= EGame::GAME_UE4_6)
            Texture.SamplerSource = static_cast<uint8>(Ar.Read<int32>());
        if (Ar.GetGame() < EGame::GAME_UE4_23)
            return Texture;
        bool bVirtualTexture = false;
        Ar << bVirtualTexture;
        return Texture;
    }

    FMaterialExternalTextureParameterInfo ReadExternalTexture(FName ParameterName) {
        FMaterialExternalTextureParameterInfo ExternalTexture;
        ExternalTexture.ParameterName = ParameterName;
        if (Ar.GetGame() >= EGame::GAME_UE4_18)
            ExternalTexture.SourceTextureIndex = Ar.Read<int32>();
        Ar << ExternalTexture.ExternalTextureGuid;
        return ExternalTexture;
    }

    static uint16 FindOrAddParameter(TArray<FNumericParameter>& Parameters, const FNumericParameter& Parameter) {
        const FHashedMaterialParameterInfo& Info = Parameter.ParameterInfo;
        int32 Index = Parameters.IndexOfByPredicate([&](const FNumericParameter& Other) {
            return Other.ParameterInfo.Name == Info.Name && Other.ParameterInfo.Association == Info.Association && Other.ParameterInfo.Index == Info.Index;
        });
        if (Index == INDEX_NONE)
            Index = Parameters.Add(Parameter);
        return static_cast<uint16>(Index);
    }

    void AddNumericParameter(const FNumericParameter& Parameter, EMaterialParameterType Type, std::size_t Size) {
        FMaterialNumericParameterInfo& Numeric = Value.UniformNumericParameters.Emplace_GetRef();
        Numeric.ParameterInfo = Parameter.ParameterInfo;
        Numeric.ParameterType = Type;
        Numeric.DefaultValueOffset = static_cast<uint32>(Value.DefaultValues.AddUninitialized(static_cast<int32>(Size)));
        std::memcpy(Value.DefaultValues.GetData() + Numeric.DefaultValueOffset, &Parameter.DefaultValue, Size);
    }

    void WriteOpcode(EPreshaderOpcode Opcode) { Write(*FindPreshaderOpcode(Ar.GetGame(), Opcode)); }

    template <class T>
    void Write(const T& Data) {
        const int32 Offset = Code->AddUninitialized(static_cast<int32>(sizeof(T)));
        std::memcpy(Code->GetData() + Offset, &Data, sizeof(T));
    }

    //! UE 4.25 writes an FHashedMaterialParameterInfo as its 16 bytes in memory: the 8-byte name hash, the int32 index, the byte association and 3 bytes of padding.
    void Write(const FHashedMaterialParameterInfo& ParameterInfo) {
        Write(FHashedName(ParameterInfo.Name).GetHash());
        Write(ParameterInfo.Index);
        Write(static_cast<uint32>(static_cast<uint8>(ParameterInfo.Association)));
    }

    FArchive& Ar;
    FUniformExpressionSet& Value;
    TArray<uint8>* Code;
    TArray<FNumericParameter> ScalarParameters;
    TArray<FNumericParameter> VectorParameters;
    bool bPerFrame = false;
    int32 Depth = 0;
};

void AddPreshaders(FUniformExpressionSet& Value, const TArray<FMaterialUniformPreshaderHeader>& Headers, uint8 Type, uint32 NumComponents,
                   uint32& BufferOffset) {
    for (FMaterialUniformPreshaderHeader Header : Headers) {
        Header.FieldIndex = static_cast<uint32>(Value.UniformPreshaderFields.Num());
        Header.NumFields = 1u;
        Value.UniformPreshaderFields.Add({BufferOffset, 0u, Type});
        Value.UniformPreshaders.Add(Header);
        BufferOffset += NumComponents;
    }
}

}

//! The material uniform buffer keeps the vector expressions first and packs the scalar expressions after them, as UE 4.25 does.
void SerializeLegacyUniformExpressionSet(FArchive& Ar, FUniformExpressionSet& Value) {
    using enum EMaterialTextureParameterType;
    FLegacyUniformExpressionReader Reader(Ar, Value);
    const EGame Game = Ar.GetGame();
    TArray<FMaterialUniformPreshaderHeader> VectorPreshaders;
    TArray<FMaterialUniformPreshaderHeader> ScalarPreshaders;
    Reader.ReadNumberExpressions(VectorPreshaders);
    Reader.ReadNumberExpressions(ScalarPreshaders);
    Reader.ReadTextureExpressions(Standard2D);
    Reader.ReadTextureExpressions(Cube);
    if (Game >= EGame::GAME_UE4_20)
        Reader.ReadTextureExpressions(Volume);
    if (Game >= EGame::GAME_UE4_23)
        Reader.ReadTextureExpressions(Virtual);
    if (Game >= EGame::GAME_UE4_17)
        Reader.ReadExternalTextureExpressions();
    if (Game >= EGame::GAME_UE4_23)
        Reader.ReadVirtualTextureStacks();
    if (Game >= EGame::GAME_UE4_20)
        Reader.ReadTextureExpressions(Array2D);
    Ar << Value.ParameterCollections;
    const int32 NumPerFrameArrays = Game >= EGame::GAME_UE4_20 ? 0 : Game >= EGame::GAME_UE4_8 ? 4 : Game >= EGame::GAME_UE4_7 ? 2 : 0;
    for (int32 Index = 0; Index < NumPerFrameArrays; Index++)
        Reader.ReadPerFrameExpressions();
    uint32 BufferOffset = 0u;
    AddPreshaders(Value, VectorPreshaders, ValueTypeFloat4, 4u, BufferOffset);
    AddPreshaders(Value, ScalarPreshaders, ValueTypeFloat1, 1u, BufferOffset);
    Value.UniformPreshaderBufferSize = (BufferOffset + 3u) / 4u;
    Reader.AddNumericParameters();
}

}
