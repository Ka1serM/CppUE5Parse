#include "Engine/Source/Runtime/Engine/Materials/MaterialUniformExpressions.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

constexpr uint64 ExpressionType(std::string_view Name) {
    return FName(Name).getHash();
}

class FLegacyUniformExpressionReader {
public:
    explicit FLegacyUniformExpressionReader(FArchive& InAr) : Ar(InAr) {}

    void ReadExpressions() {
        int32 NumExpressions = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumExpressions && !Ar.IsError(); Index++)
            ReadExpression();
    }

    void ReadVirtualTextureStacks() {
        int32 NumStacks = Ar.ReadCount(8);
        for (int32 Index = 0; Index < NumStacks && !Ar.IsError(); Index++) {
            uint32 NumLayers = Ar.Read<uint32>();
            Ar.SkipBytes(static_cast<int64>(NumLayers) * static_cast<int64>(sizeof(int32)));
            Ar.Read<int32>();
        }
    }

private:
    static constexpr int32 MaxDepth = 256;

    void ReadExpression() {
        if (Depth >= MaxDepth) {
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.Depth");
            return;
        }
        Depth++;
        FName TypeName;
        Ar << TypeName;
        if (!Ar.IsError())
            ReadExpressionData(TypeName);
        Depth--;
    }

    void ReadExpressionData(FName TypeName) {
        switch (TypeName.getHash()) {
        case ExpressionType("FMaterialUniformExpressionAbs"):
        case ExpressionType("FMaterialUniformExpressionCeil"):
        case ExpressionType("FMaterialUniformExpressionFloor"):
        case ExpressionType("FMaterialUniformExpressionFrac"):
        case ExpressionType("FMaterialUniformExpressionPeriodic"):
        case ExpressionType("FMaterialUniformExpressionSquareRoot"):
        case ExpressionType("FMaterialUniformExpressionRound"):
        case ExpressionType("FMaterialUniformExpressionTruncate"):
        case ExpressionType("FMaterialUniformExpressionSign"):
        case ExpressionType("FMaterialUniformExpressionLogarithm2"):
        case ExpressionType("FMaterialUniformExpressionLogarithm10"):
        case ExpressionType("FMaterialUniformExpressionSaturate"):
            ReadExpression();
            return;
        case ExpressionType("FMaterialUniformExpressionLength"):
            ReadExpression();
            ReadSqrtValueType();
            return;
        case ExpressionType("FMaterialUniformExpressionMin"):
        case ExpressionType("FMaterialUniformExpressionMax"):
        case ExpressionType("FMaterialUniformExpressionFmod"):
            ReadExpression();
            ReadExpression();
            return;
        case ExpressionType("FMaterialUniformExpressionAppendVector"):
            ReadExpression();
            ReadExpression();
            Ar.Read<uint32>();
            return;
        case ExpressionType("FMaterialUniformExpressionClamp"):
            ReadExpression();
            ReadExpression();
            ReadExpression();
            return;
        case ExpressionType("FMaterialUniformExpressionComponentSwizzle"):
            ReadExpression();
            Ar.SkipBytes(5);
            return;
        case ExpressionType("FMaterialUniformExpressionConstant"): {
            FLinearColor Value;
            Ar << Value;
            Ar.Read<uint8>();
            return;
        }
        case ExpressionType("FMaterialUniformExpressionFoldedMath"):
            ReadExpression();
            ReadExpression();
            Ar.Read<uint8>();
            ReadSqrtValueType();
            return;
        case ExpressionType("FMaterialUniformExpressionSine"): {
            ReadExpression();
            bool bIsCosine = false;
            Ar << bIsCosine;
            return;
        }
        case ExpressionType("FMaterialUniformExpressionTrigMath"):
            ReadExpression();
            ReadExpression();
            Ar.Read<uint8>();
            return;
        case ExpressionType("FMaterialUniformExpressionTime"):
        case ExpressionType("FMaterialUniformExpressionRealTime"):
            return;
        case ExpressionType("FMaterialUniformExpressionScalarParameter"):
            ReadParameterName();
            Ar.Read<float>();
            return;
        case ExpressionType("FMaterialUniformExpressionVectorParameter"): {
            ReadParameterName();
            FLinearColor DefaultValue;
            Ar << DefaultValue;
            return;
        }
        case ExpressionType("FMaterialUniformExpressionTexture"):
        case ExpressionType("FMaterialUniformExpressionFlipBookTextureParameter"):
            ReadTexture();
            return;
        case ExpressionType("FMaterialUniformExpressionTextureParameter"):
            ReadParameterName();
            ReadTexture();
            return;
        case ExpressionType("FMaterialUniformExpressionTextureProperty"):
            ReadExpression();
            Ar.Read<int8>();
            return;
        case ExpressionType("FMaterialUniformExpressionExternalTexture"):
            ReadExternalTexture();
            return;
        case ExpressionType("FMaterialUniformExpressionExternalTextureParameter"): {
            FName ParameterName;
            Ar << ParameterName;
            ReadExternalTexture();
            return;
        }
        case ExpressionType("FMaterialUniformExpressionExternalTextureCoordinateScaleRotation"):
        case ExpressionType("FMaterialUniformExpressionExternalTextureCoordinateOffset"): {
            bool bHasParameterName = false;
            Ar << bHasParameterName;
            FName ParameterName;
            if (bHasParameterName)
                Ar << ParameterName;
            ReadExternalTexture();
            return;
        }
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureParameter"):
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureParameter_DEPRECATED"):
            Ar.Read<int32>();
            Ar.Read<int32>();
            return;
        case ExpressionType("FMaterialUniformExpressionRuntimeVirtualTextureUniform"): {
            bool bParameter = false;
            Ar << bParameter;
            ReadParameterInfo();
            Ar.Read<int32>();
            Ar.Read<int32>();
            return;
        }
        default:
            Ar.SetError(ErrorCode::InvalidData, "FMaterialUniformExpression.Type");
            return;
        }
    }

    void ReadSqrtValueType() {
        if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TypeHandlingForMaterialSqrtNodes, EGame::GAME_UE4_14))
            Ar.Read<uint32>();
    }

    void ReadParameterInfo() {
        FName Name;
        Ar << Name;
        Ar.Read<uint8>();
        Ar.Read<int32>();
    }

    void ReadParameterName() {
        if (Ar.GetGame() >= EGame::GAME_UE4_19) {
            ReadParameterInfo();
            return;
        }
        FName Name;
        Ar << Name;
    }

    void ReadTexture() {
        Ar.Read<int32>();
        if (Ar.GetGame() >= EGame::GAME_UE4_24) {
            Ar.Read<int16>();
            Ar.Read<int16>();
        } else if (Ar.GetGame() >= EGame::GAME_UE4_23) {
            Ar.Read<int32>();
        }
        if (Ar.GetGame() >= EGame::GAME_UE4_6)
            Ar.Read<int32>();
        if (Ar.GetGame() < EGame::GAME_UE4_23)
            return;
        bool bVirtualTexture = false;
        Ar << bVirtualTexture;
    }

    void ReadExternalTexture() {
        if (Ar.GetGame() >= EGame::GAME_UE4_18)
            Ar.Read<int32>();
        FGuid ExternalTextureGuid;
        Ar << ExternalTextureGuid;
    }

    FArchive& Ar;
    int32 Depth = 0;
};

}

void SerializeLegacyUniformExpressionSet(FArchive& Ar, FUniformExpressionSet& Value) {
    FLegacyUniformExpressionReader Reader(Ar);
    const EGame Game = Ar.GetGame();
    for (int32 Index = 0; Index < 4; Index++)
        Reader.ReadExpressions();
    if (Game >= EGame::GAME_UE4_20)
        Reader.ReadExpressions();
    if (Game >= EGame::GAME_UE4_23)
        Reader.ReadExpressions();
    if (Game >= EGame::GAME_UE4_17)
        Reader.ReadExpressions();
    if (Game >= EGame::GAME_UE4_23)
        Reader.ReadVirtualTextureStacks();
    if (Game >= EGame::GAME_UE4_20)
        Reader.ReadExpressions();
    Ar << Value.ParameterCollections;
    if (Game >= EGame::GAME_UE4_20)
        return;
    const int32 NumPerFrameArrays = Game >= EGame::GAME_UE4_8 ? 4 : Game >= EGame::GAME_UE4_7 ? 2 : 0;
    for (int32 Index = 0; Index < NumPerFrameArrays; Index++)
        Reader.ReadExpressions();
}

}
