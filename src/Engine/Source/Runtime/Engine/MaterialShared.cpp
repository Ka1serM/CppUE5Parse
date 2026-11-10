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
    Reader.ReadArray(Value.Names, 4u, [](FMemoryImageReader& Element, FName& Name) { Name = Element.ReadScriptName(); });
    Reader.ReadArray(Value.StructTypes);
    Reader.ReadArray(Value.StructComponentTypes);
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

void Unfreeze(FMemoryImageReader& Reader, FStrataMaterialCompilationOutput& Value) {
    Value.StrataMaterialType = Reader.Read<uint8>();
    Value.StrataBSDFCount = Reader.Read<uint8>();
    Value.StrataUintPerPixel = Reader.Read<uint8>();
    Value.bUsesComplexSpecialRenderPath = (Reader.Read<uint8>() & 1u) != 0u;
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialCompilationOutput& Value) {
    Reader.ReadStruct(Value.UniformExpressionSet);
    Value.UsedSceneTextures = Reader.Read<uint32>();
    Value.UsedPathTracingBufferTextures = Reader.Read<uint8>();
    Reader.ReadStruct(Value.StrataMaterialCompilationOutput);
    Value.UsedDBufferTextures = Reader.Read<uint8>();
    Value.RuntimeVirtualTextureOutputAttributeMask = Reader.Read<uint8>();
    uint8 Low = Reader.Read<uint8>();
    uint8 High = Reader.Read<uint8>();
    Value.bNeedsSceneTextures = (Low & (1u << 0)) != 0u;
    Value.bUsesDBufferTextureLookup = (Low & (1u << 1)) != 0u;
    Value.bUsesEyeAdaptation = (Low & (1u << 2)) != 0u;
    Value.bModifiesMeshPosition = (Low & (1u << 3)) != 0u;
    Value.bUsesWorldPositionOffset = (Low & (1u << 4)) != 0u;
    Value.bUsesGlobalDistanceField = (Low & (1u << 5)) != 0u;
    Value.bUsesPixelDepthOffset = (Low & (1u << 6)) != 0u;
    Value.bUsesDistanceCullFade = (Low & (1u << 7)) != 0u;
    Value.bUsesPerInstanceCustomData = (High & (1u << 0)) != 0u;
    Value.bUsesPerInstanceRandom = (High & (1u << 1)) != 0u;
    Value.bUsesVertexInterpolator = (High & (1u << 2)) != 0u;
    Value.bHasRuntimeVirtualTextureOutputNode = (High & (1u << 3)) != 0u;
    Value.bUsesAnisotropy = (High & (1u << 4)) != 0u;
    Value.bUsesDisplacement = (High & (1u << 5)) != 0u;
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
    Value.ShaderContentHash = Reader.Read<FSHAHash>();
}

void FMaterialShaderMapId::Serialize(FArchive& Ar) {
    uint8 Quality = 0u;
    int32 Feature = 0;
    Ar << Quality << Feature << CookedShaderMapIdHash << LayoutParams;
    QualityLevel = static_cast<EMaterialQualityLevel>(Quality);
    FeatureLevel = static_cast<ERHIFeatureLevel::Type>(Feature);
}

void FMaterialShaderMap::Serialize(FArchive& Ar) {
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
