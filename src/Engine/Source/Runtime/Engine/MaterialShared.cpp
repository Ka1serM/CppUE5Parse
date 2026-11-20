#include "Engine/Source/Runtime/Engine/MaterialShared.h"

namespace ue5parse::runtime {

static_assert(sizeof(FMaterialUniformPreshaderHeader) == 16 && sizeof(FMaterialUniformPreshaderField) == 12 && sizeof(FMaterialVirtualTextureStack) == 40
              && sizeof(FPreshaderStructType) == 16 && sizeof(FRHIUniformBufferResourceInitializer) == 4);

void Unfreeze(FMemoryImageReader& Reader, FHashedMaterialParameterInfo& Value) {
    Value.Name = Reader.ReadScriptName();
    Value.Index = Reader.Read<int32>();
    Value.Association = static_cast<EMaterialParameterAssociation>(Reader.Read<uint8>());
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialNumericParameterInfo& Value) {
    Reader.ReadStruct(Value.ParameterInfo);
    Value.ParameterType = Reader.Read<EMaterialParameterType>();
    Value.DefaultValueOffset = Reader.Read<uint32>();
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialTextureParameterInfo& Value) {
    Reader.ReadStruct(Value.ParameterInfo);
    Value.TextureIndex = Reader.Read<int32>();
    Value.SamplerSource = Reader.Read<uint8>();
    Value.VirtualTextureLayerIndex = Reader.Read<uint8>();
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialExternalTextureParameterInfo& Value) {
    Value.ParameterName = Reader.ReadScriptName();
    Value.ExternalTextureGuid = Reader.Read<FGuid>();
    Value.SourceTextureIndex = Reader.Read<int32>();
}

void Unfreeze(FMemoryImageReader& Reader, FPreshaderData& Value) {
    const bool bHasPreshader2 = Reader.GetGame() >= EGame::GAME_UE5_8;
    if (bHasPreshader2) {
        Value.bPreshader2 = Reader.Read<uint8>() != 0u;
        Value.bPreFixup = Reader.Read<uint8>() != 0u;
        Value.Preshader2TemporarySize = Reader.Read<uint16>();
    }
    if (Reader.GetGame() >= EGame::GAME_UE5_9)
        Value.bQuantizePreshaderToHalf = Reader.Read<uint8>() != 0u;
    Reader.ReadArray(Value.Names, 4u, [](FMemoryImageReader& Element, FName& Name) { Name = Element.ReadScriptName(); });
    if (!bHasPreshader2) {
        Reader.ReadArray(Value.StructTypes);
        Reader.ReadArray(Value.StructComponentTypes);
    }
    Reader.ReadArray(Value.Data);
}

void Unfreeze(FMemoryImageReader& Reader, FRHIUniformBufferLayoutInitializer& Value) {
    TArray<char16_t> Name;
    Reader.ReadArray(Name);
    Value.Name = FString(std::u16string_view(Name.GetData(), Name.Num() > 0 ? static_cast<std::size_t>(Name.Num() - 1) : 0u));
    Reader.ReadArray(Value.Resources);
    Reader.ReadArray(Value.GraphResources);
    Reader.ReadArray(Value.GraphTextures);
    Reader.ReadArray(Value.GraphBuffers);
    Reader.ReadArray(Value.GraphUniformBuffers);
    Reader.ReadArray(Value.UniformBuffers);
    Value.Hash = Reader.Read<uint32>();
    Value.ConstantBufferSize = Reader.Read<uint32>();
    Value.RenderTargetsOffset = Reader.Read<uint16>();
    Value.StaticSlot = Reader.Read<uint8>();
    Value.BindingFlags = Reader.Read<uint8>();
    Value.bHasNonGraphOutputs = Reader.Read<uint8>() != 0u;
    Value.bNoEmulatedUniformBuffer = Reader.Read<uint8>() != 0u;
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialTextureCollectionParameterInfo& Value) {
    Reader.ReadStruct(Value.ParameterInfo);
    Value.TextureCollectionIndex = Reader.Read<int32>();
    if (Reader.GetGame() >= EGame::GAME_UE5_7)
        Value.bIsVirtualCollection = Reader.Read<uint8>() != 0u;
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialUniformParameterEvaluation& Value) {
    Value.ParameterIndex = Reader.Read<uint16>();
    Value.BufferOffset = Reader.Read<uint16>();
    if (Reader.GetGame() >= EGame::GAME_UE5_8)
        Value.ComponentCount = Reader.Read<uint16>();
}

void Unfreeze(FMemoryImageReader& Reader, FUniformExpressionSet& Value) {
    const EGame Game = Reader.GetGame();
    if (Game >= EGame::GAME_UE5_6)
        Reader.ReadArray(Value.UniformParameterEvaluations);
    if (Game >= EGame::GAME_UE5_8) {
        //! UE 5.8 merges the preshader fields into the headers: each header writes its whole type at its buffer offset.
        Reader.ReadArray(Value.UniformPreshaders, 4u, [&](FMemoryImageReader& Element, FMaterialUniformPreshaderHeader& Header) {
            const uint8 Type = Element.Read<uint8>();
            const uint16 BufferOffset = Element.Read<uint16>();
            Header.OpcodeOffset = Element.Read<uint32>();
            Header.OpcodeSize = Element.Read<uint32>();
            Header.FieldIndex = static_cast<uint32>(Value.UniformPreshaderFields.Num());
            Header.NumFields = 1u;
            Value.UniformPreshaderFields.Add({BufferOffset, 0u, Type});
        });
    } else {
        Reader.ReadArray(Value.UniformPreshaders);
        Reader.ReadArray(Value.UniformPreshaderFields);
    }
    Reader.ReadArray(Value.UniformNumericParameters);
    for (TArray<FMaterialTextureParameterInfo>& Parameters : Value.UniformTextureParameters)
        Reader.ReadArray(Parameters);
    Reader.ReadArray(Value.UniformExternalTextureParameters);
    if (Game >= EGame::GAME_UE5_5)
        Reader.ReadArray(Value.UniformTextureCollectionParameters);
    Value.UniformPreshaderBufferSize = Reader.Read<uint32>();
    if (Game >= EGame::GAME_UE5_9) {
        Value.UniformPreshaderHalfHead = Reader.Read<uint16>();
        Value.UniformPreshaderHalfTail = Reader.Read<uint16>();
    }
    Reader.ReadStruct(Value.UniformPreshaderData);
    Reader.ReadArray(Value.DefaultValues);
    Reader.ReadArray(Value.VTStacks);
    if (Game >= EGame::GAME_UE5_7)
        Reader.ReadArray(Value.MaterialCacheTagStacks);
    Reader.ReadArray(Value.ParameterCollections);
    Reader.ReadStruct(Value.UniformBufferLayoutInitializer);
    if (Game >= EGame::GAME_UE5_8)
        Reader.SkipArray();
}

namespace {

using FCompilationFlag = bool FMaterialCompilationOutput::*;
using O = FMaterialCompilationOutput;

constexpr FCompilationFlag CompilationFlagsUE5_0[] = {&O::bNeedsSceneTextures,       &O::bUsesEyeAdaptation,        &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,  &O::bUsesGlobalDistanceField,  &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,     &O::bUsesPerInstanceCustomData, &O::bUsesPerInstanceRandom,
                                                      &O::bUsesVertexInterpolator,   &O::bHasRuntimeVirtualTextureOutputNode, &O::bUsesAnisotropy};
constexpr FCompilationFlag CompilationFlagsUE5_3[] = {
    &O::bNeedsSceneTextures,       &O::bUsesDBufferTextureLookup, &O::bUsesEyeAdaptation,     &O::bModifiesMeshPosition,
    &O::bUsesWorldPositionOffset,  &O::bUsesGlobalDistanceField,  &O::bUsesPixelDepthOffset,  &O::bUsesDistanceCullFade,
    &O::bUsesPerInstanceCustomData, &O::bUsesPerInstanceRandom,   &O::bUsesVertexInterpolator, &O::bHasRuntimeVirtualTextureOutputNode,
    &O::bUsesAnisotropy,           &O::bUsesDisplacement};
constexpr FCompilationFlag CompilationFlagsUE5_4[] = {
    &O::bNeedsSceneTextures,       &O::bUsesDBufferTextureLookup, &O::bUsesEyeAdaptation,       &O::bModifiesMeshPosition,
    &O::bUsesWorldPositionOffset,  &O::bUsesGlobalDistanceField,  &O::bUsesPixelDepthOffset,    &O::bUsesDistanceCullFade,
    &O::bUsesPerInstanceCustomData, &O::bUsesVertexInterpolator,  &O::bHasRuntimeVirtualTextureOutputNode, &O::bUsesAnisotropy,
    &O::bUsesDisplacement,         &O::bUsedWithNeuralNetworks};
constexpr FCompilationFlag CompilationFlagsUE5_5[] = {
    &O::bNeedsSceneTextures,       &O::bUsesDBufferTextureLookup, &O::bUsesEyeAdaptation,       &O::bModifiesMeshPosition,
    &O::bUsesWorldPositionOffset,  &O::bUsesGlobalDistanceField,  &O::bUsesPixelDepthOffset,    &O::bUsesDistanceCullFade,
    &O::bUsesPerInstanceCustomData, &O::bUsesVertexInterpolator,  &O::bHasRuntimeVirtualTextureOutputNode, &O::bUsesAnisotropy,
    &O::bIsLightFunctionAtlasCompatible, &O::bUsesDisplacement,   &O::bUsedWithNeuralNetworks,  &O::bUsesCustomizedUVs};
constexpr FCompilationFlag CompilationFlagsUE5_7[] = {
    &O::bNeedsSceneTextures,       &O::bUsesDBufferTextureLookup,   &O::bUsesEyeAdaptation,       &O::bModifiesMeshPosition,
    &O::bUsesWorldPositionOffset,  &O::bUsesTemporalResponsiveness, &O::bUsesMotionVectorWorldOffset, &O::bUsesGlobalDistanceField,
    &O::bUsesPixelDepthOffset,     &O::bUsesDistanceCullFade,       &O::bUsesPerInstanceCustomData, &O::bUsesVertexInterpolator,
    &O::bHasRuntimeVirtualTextureOutputNode, &O::bUsesAnisotropy,   &O::bIsLightFunctionAtlasCompatible, &O::bUsesDisplacement,
    &O::bUsedWithNeuralNetworks,   &O::bUsesCustomizedUVs};
constexpr FCompilationFlag CompilationFlagsUE5_8[] = {
    &O::bNeedsSceneTextures,       &O::bUsesDBufferTextureLookup,   &O::bUsesEyeAdaptation,       &O::bModifiesMeshPosition,
    &O::bUsesWorldPositionOffset,  &O::bUsesTemporalResponsiveness, &O::bUsesMotionVectorWorldOffset, &O::bUsesGlobalDistanceField,
    &O::bUsesPixelDepthOffset,     &O::bUsesDistanceCullFade,       &O::bUsesPerInstanceCustomData, &O::bUsesVertexInterpolator,
    &O::bHasRuntimeVirtualTextureOutputNode, &O::bUsesAnisotropy,   &O::bIsLightFunctionAtlasCompatible, &O::bUsesDisplacement,
    &O::bUsedWithNeuralNetworks,   &O::bUsesCustomizedUVs,          &O::bUsesOpacityMask,         &O::bUsesCustomHLSLPixelDiscard,
    &O::bUsesOpacity};

std::span<const FCompilationFlag> GetCompilationFlags(EGame Game) {
    if (Game >= EGame::GAME_UE5_8)
        return CompilationFlagsUE5_8;
    if (Game >= EGame::GAME_UE5_7)
        return CompilationFlagsUE5_7;
    if (Game >= EGame::GAME_UE5_5)
        return CompilationFlagsUE5_5;
    if (Game >= EGame::GAME_UE5_4)
        return CompilationFlagsUE5_4;
    if (Game >= EGame::GAME_UE5_3)
        return CompilationFlagsUE5_3;
    return CompilationFlagsUE5_0;
}

void ReadCompilationFlags(FMemoryImageReader& Reader, FMaterialCompilationOutput& Value) {
    const std::span<const FCompilationFlag> Flags = GetCompilationFlags(Reader.GetGame());
    uint8 Bits = 0u;
    for (std::size_t Index = 0; Index < Flags.size(); Index++) {
        if (Index % 8u == 0u)
            Bits = Reader.Read<uint8>();
        Value.*Flags[Index] = (Bits & (1u << (Index % 8u))) != 0u;
    }
}

void ReadStrataOutput(FMemoryImageReader& Reader, FStrataMaterialCompilationOutput& Value) {
    const EGame Game = Reader.GetGame();
    if (Game >= EGame::GAME_UE5_7)
        Reader.Align(alignof(uint16));
    Value.StrataMaterialType = Reader.Read<uint8>();
    Value.StrataBSDFCount = Reader.Read<uint8>();
    Value.StrataUintPerPixel = Reader.Read<uint8>();
    if (Game < EGame::GAME_UE5_4)
        Value.bUsesComplexSpecialRenderPath = (Reader.Read<uint8>() & 1u) != 0u;
    if (Game >= EGame::GAME_UE5_7)
        Value.SubstrateMaterialBsdfFeatures = Reader.Read<uint16>();
}

}

void Unfreeze(FMemoryImageReader& Reader, FMaterialCompilationOutput& Value) {
    const EGame Game = Reader.GetGame();
    Reader.ReadStruct(Value.UniformExpressionSet);
    if (Game >= EGame::GAME_UE5_5)
        Reader.SkipArray();
    Value.UsedSceneTextures = Game >= EGame::GAME_UE5_9 ? Reader.Read<uint64>() : Reader.Read<uint32>();
    if (Game >= EGame::GAME_UE5_3) {
        Value.UsedPathTracingBufferTextures = Reader.Read<uint8>();
        ReadStrataOutput(Reader, Value.StrataMaterialCompilationOutput);
    }
    Value.UsedDBufferTextures = Reader.Read<uint8>();
    Value.RuntimeVirtualTextureOutputAttributeMask = Reader.Read<uint8>();
    ReadCompilationFlags(Reader, Value);
    if (Game == EGame::GAME_UE5_2) {
        Value.StrataMaterialCompilationOutput.StrataMaterialType = Reader.Read<uint8>();
        Value.StrataMaterialCompilationOutput.StrataBSDFCount = Reader.Read<uint8>();
        Value.StrataMaterialCompilationOutput.StrataUintPerPixel = Reader.Read<uint8>();
    }
}

void Unfreeze(FMemoryImageReader& Reader, FMeshMaterialShaderMap& Value) {
    Reader.ReadBase(static_cast<FShaderMapContent&>(Value));
    Value.VertexFactoryTypeName = Reader.Read<FHashedName>();
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialShaderMapContent& Value) {
    Reader.ReadBase(static_cast<FShaderMapContent&>(Value));
    Reader.ReadArray(Value.OrderedMeshShaderMaps, FMeshMaterialShaderMap::FrozenAlignment,
                     [](FMemoryImageReader& Element, FMeshMaterialShaderMap& MeshShaderMap) {
                         std::optional<FMemoryImageReader> Target = Element.ReadPointer();
                         if (!Target) {
                             Element.SetError("FMaterialShaderMapContent.OrderedMeshShaderMaps");
                             return;
                         }
                         Target->ReadStruct(MeshShaderMap);
                     });
    Reader.ReadStruct(Value.MaterialCompilationOutput);
    if (Reader.GetGame() >= EGame::GAME_UE5_8)
        Value.ShaderContentHash = Reader.Read<FShaderHash>();
    else
        Value.ShaderContentHash = Reader.Read<FSHAHash>();
}

void FMaterialShaderMapId::Serialize(FArchive& Ar) {
    uint8 Quality = 0u;
    int32 Feature = 0;
    Ar << Quality << Feature;
    QualityLevel = static_cast<EMaterialQualityLevel>(Quality);
    FeatureLevel = static_cast<ERHIFeatureLevel::Type>(Feature);
    if (Ar.GetGame() >= EGame::GAME_UE5_8)
        Ar << CookedShaderMapIdHash.emplace<uint64>();
    else
        Ar << CookedShaderMapIdHash.emplace<FSHAHash>();
    Ar << LayoutParams;
}

void FMaterialShaderMap::Serialize(FArchive& Ar) {
    //! UE 5.9 writes the shader map id after the shader map content, because the id hash includes the content hash.
    if (Ar.GetGame() >= EGame::GAME_UE5_9) {
        TShaderMap::Serialize(Ar);
        ShaderMapId.Serialize(Ar);
        return;
    }
    ShaderMapId.Serialize(Ar);
    TShaderMap::Serialize(Ar);
}

void FMaterialResource::SerializeInlineShaderMap(FArchive& Ar) {
    bool bCooked = false;
    Ar << bCooked;
    if (!Ar.IsError() && !bCooked) {
        Ar.SetError(ErrorCode::InvalidData, "FMaterial.SerializeInlineShaderMap.bCooked");
        return;
    }
    bool bValid = false;
    Ar << bValid;
    if (Ar.IsError() || !bValid)
        return;
    ShaderMap.emplace().Serialize(Ar);
}

namespace {

class FMaterialResourceProxyReader : public FArchive {
public:
    explicit FMaterialResourceProxyReader(FArchive& InInnerArchive) : InnerArchive(InInnerArchive) {
        CopyStateFrom(InInnerArchive);
        Initialize();
    }

    void Serialize(void* Data, int64 Length) override {
        InnerArchive.Serialize(Data, Length);
        TakeInnerError();
    }
    int64 Tell() const override { return InnerArchive.Tell(); }
    int64 TotalSize() const override { return InnerArchive.TotalSize(); }
    void Seek(int64 InPos) override {
        InnerArchive.Seek(InPos);
        TakeInnerError();
    }
    std::span<const std::byte> ReadView(int64 Length) override {
        std::span<const std::byte> View = InnerArchive.ReadView(Length);
        TakeInnerError();
        return View;
    }

    using FArchive::operator<<;
    FArchive& operator<<(FName& Name) override {
        int32 NameIdx = 0;
        int32 InstNum = 0;
        *this << NameIdx << InstNum;
        if (IsError())
            return *this;
        if (NameIdx < 0 || NameIdx >= Names.Num() || InstNum < 0) {
            SetError(ErrorCode::InvalidData, "FMaterialResourceProxyReader.Name");
            return *this;
        }
        Name = FName(Names[NameIdx].getText(), Names[NameIdx].getHash(), static_cast<uint32>(InstNum));
        return *this;
    }

private:
    struct FMaterialResourceLocOnDisk {
        uint32 Offset = 0u;
        uint8 FeatureLevel = 0u;
        uint8 QualityLevel = 0u;
    };

    void TakeInnerError() {
        if (InnerArchive.IsError())
            SetError(InnerArchive.GetError().code, InnerArchive.GetError().where);
    }

    void Initialize() {
        int32 NumNames = ReadCount(4);
        Names.Empty(NumNames);
        for (int32 Index = 0; Index < NumNames && !IsError(); Index++) {
            FString Text;
            *this << Text;
            Read<uint32>();
            Names.Add(MakeName(Text));
        }
        int32 NumLocs = ReadCount(6);
        for (int32 Index = 0; Index < NumLocs && !IsError(); Index++) {
            FMaterialResourceLocOnDisk Loc;
            *this << Loc.Offset << Loc.FeatureLevel << Loc.QualityLevel;
            if (Index == 0 && Loc.Offset != 0u)
                SetError(ErrorCode::InvalidData, "FMaterialResourceProxyReader.Locs");
        }
        Read<int32>();
    }

    FArchive& InnerArchive;
    TArray<FName> Names;
};

}

void SerializeInlineShaderMaps(FArchive& Ar, TArray<FMaterialResource>& OutLoadedResources) {
    int32 NumLoadedResources = Ar.ReadCount(8);
    OutLoadedResources.Empty(NumLoadedResources);
    if (Ar.IsError() || NumLoadedResources == 0)
        return;
    FMaterialResourceProxyReader ResourceAr(Ar);
    for (int32 ResourceIndex = 0; ResourceIndex < NumLoadedResources && !ResourceAr.IsError(); ++ResourceIndex)
        OutLoadedResources[OutLoadedResources.AddDefaulted()].SerializeInlineShaderMap(ResourceAr);
    if (ResourceAr.IsError())
        Ar.SetError(ResourceAr.GetError().code, ResourceAr.GetError().where);
}

}
