#include "Engine/Source/Runtime/Engine/MaterialShared.h"

#include <utility>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialUniformExpressions.h"

namespace ue5parse::runtime {

static_assert(sizeof(FMaterialUniformPreshaderHeader) == 16 && sizeof(FMaterialUniformPreshaderField) == 12 && sizeof(FMaterialVirtualTextureStack) == 40
              && sizeof(FPreshaderStructType) == 16 && sizeof(FRHIUniformBufferResourceInitializer) == 4);

void Unfreeze(FMemoryImageReader& Reader, FHashedMaterialParameterInfo& Value) {
    Value.Name = Reader.ReadScriptName();
    Value.Index = Reader.Read<int32>();
    Value.Association = static_cast<EMaterialParameterAssociation>(Reader.Read<uint8>());
}

namespace {

struct FHashedMaterialParameterInfoUE4_25 {
    uint64 NameHash;
    int32 Index;
    uint8 Association;
};

//! UE 4.25 keeps only the hash of the name in the parameter info and the name as a string after the info.
FHashedMaterialParameterInfo ReadParameterInfo(FMemoryImageReader& Reader) {
    FHashedMaterialParameterInfo Info;
    if (Reader.GetGame() >= EGame::GAME_UE4_26) {
        Reader.ReadStruct(Info);
        return Info;
    }
    const FHashedMaterialParameterInfoUE4_25 Frozen = Reader.Read<FHashedMaterialParameterInfoUE4_25>();
    Info.Index = Frozen.Index;
    Info.Association = static_cast<EMaterialParameterAssociation>(Frozen.Association);
    Info.Name = MakeName(Reader.ReadMemoryImageString());
    return Info;
}

uint32 GetParameterAlignment(FMemoryImageReader& Reader) {
    return Reader.GetGame() >= EGame::GAME_UE4_26 ? 4u : 8u;
}

}

void Unfreeze(FMemoryImageReader& Reader, FMaterialNumericParameterInfo& Value) {
    Reader.ReadStruct(Value.ParameterInfo);
    Value.ParameterType = Reader.Read<EMaterialParameterType>();
    Value.DefaultValueOffset = Reader.Read<uint32>();
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialTextureParameterInfo& Value) {
    Value.ParameterInfo = ReadParameterInfo(Reader);
    Value.TextureIndex = Reader.Read<int32>();
    Value.SamplerSource = Reader.Read<uint8>();
    Value.VirtualTextureLayerIndex = Reader.Read<uint8>();
}

void Unfreeze(FMemoryImageReader& Reader, FMaterialExternalTextureParameterInfo& Value) {
    //! UE 4.25 keeps only the hash of the external texture parameter name.
    if (Reader.GetGame() < EGame::GAME_UE4_26)
        Reader.Read<FHashedName>();
    else
        Value.ParameterName = Reader.ReadScriptName();
    Value.ExternalTextureGuid = Reader.Read<FGuid>();
    Value.SourceTextureIndex = Reader.Read<int32>();
}

void Unfreeze(FMemoryImageReader& Reader, FPreshaderData& Value) {
    if (Reader.GetGame() == EGame::GAME_DuneAwakening)
        Reader.Skip(56);
    const bool bHasPreshader2 = Reader.GetGame() >= EGame::GAME_UE5_8;
    if (bHasPreshader2) {
        Value.bPreshader2 = Reader.Read<uint8>() != 0u;
        Value.bPreFixup = Reader.Read<uint8>() != 0u;
        Value.Preshader2TemporarySize = Reader.Read<uint16>();
    }
    if (Reader.GetGame() >= EGame::GAME_UE5_9)
        Value.bQuantizePreshaderToHalf = Reader.Read<uint8>() != 0u;
    if (Reader.GetGame() >= EGame::GAME_UE4_26)
        Reader.ReadArray(Value.Names, 4u, [](FMemoryImageReader& Element, FName& Name) { Name = Element.ReadScriptName(); });
    if (Reader.GetGame() >= EGame::GAME_UE5_0 && Reader.GetGame() < EGame::GAME_UE5_1) {
        Reader.ReadArray(Value.NameOffsets);
    } else if (Reader.GetGame() >= EGame::GAME_UE5_1 && !bHasPreshader2) {
        Reader.ReadArray(Value.StructTypes);
        Reader.ReadArray(Value.StructComponentTypes);
    }
    if (Reader.GetGame() == EGame::GAME_HogwartsLegacy)
        Reader.Skip(96);
    Reader.ReadArray(Value.Data);
}

namespace {

void UnfreezeUE4(FMemoryImageReader& Reader, FRHIUniformBufferLayoutInitializer& Value) {
    const bool bHasGraphResources = Reader.GetGame() >= EGame::GAME_UE4_26;
    Value.ConstantBufferSize = Reader.Read<uint32>();
    Value.StaticSlot = Reader.Read<uint8>();
    if (bHasGraphResources) {
        Value.RenderTargetsOffset = Reader.Read<uint16>();
        Value.bHasNonGraphOutputs = Reader.Read<uint8>() != 0u;
    }
    Reader.ReadArray(Value.Resources);
    if (bHasGraphResources) {
        Reader.ReadArray(Value.GraphResources);
        Reader.ReadArray(Value.GraphTextures);
        Reader.ReadArray(Value.GraphBuffers);
        Reader.ReadArray(Value.GraphUniformBuffers);
        Reader.ReadArray(Value.UniformBuffers);
    }
    Reader.Read<int32>();
    Value.Name = Reader.ReadMemoryImageString();
    Value.Hash = Reader.Read<uint32>();
}

}

void Unfreeze(FMemoryImageReader& Reader, FRHIUniformBufferLayoutInitializer& Value) {
    if (Reader.GetGame() < EGame::GAME_UE5_0) {
        UnfreezeUE4(Reader, Value);
        return;
    }
    Value.Name = Reader.ReadMemoryImageString();
    Reader.ReadArray(Value.Resources);
    Reader.ReadArray(Value.GraphResources);
    Reader.ReadArray(Value.GraphTextures);
    Reader.ReadArray(Value.GraphBuffers);
    Reader.ReadArray(Value.GraphUniformBuffers);
    Reader.ReadArray(Value.UniformBuffers);
    Value.Hash = Reader.Read<uint32>();
    Value.ConstantBufferSize = Reader.Read<uint32>();
    if (Reader.GetGame() == EGame::GAME_FateTrigger)
        Reader.Skip(4);
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

namespace {

constexpr uint32 NumMaterialTextureParameterTypesUE5_0 = static_cast<uint32>(EMaterialTextureParameterType::SparseVolume);

uint8 GetValueTypeUE5_0(FMemoryImageReader& Reader, uint8 ComponentType, uint8 NumComponents) {
    constexpr uint8 Float = 1u;
    constexpr uint8 Bool = 4u;
    if (ComponentType < Float || ComponentType > Bool || NumComponents < 1u || NumComponents > 4u) {
        Reader.SetError("FMaterialUniformPreshaderHeader.ComponentType");
        return 0u;
    }
    //! UE 5.1 EValueType lists Float1-4, Double1-4, Int1-4 and Bool1-4 after Void, in the order of the UE 5.0 EValueComponentType.
    return static_cast<uint8>(1u + (ComponentType - Float) * 4u + (NumComponents - 1u));
}

}

namespace {

constexpr uint8 ValueTypeFloat1 = 1u;
constexpr uint8 ValueTypeFloat4 = 4u;
constexpr EMaterialTextureParameterType TextureParameterTypesUE4[] = {EMaterialTextureParameterType::Standard2D, EMaterialTextureParameterType::Cube,
                                                                      EMaterialTextureParameterType::Array2D, EMaterialTextureParameterType::Volume,
                                                                      EMaterialTextureParameterType::Virtual};

void ReadPreshaderHeadersUE4(FMemoryImageReader& Reader, TArray<FMaterialUniformPreshaderHeader>& Out) {
    Reader.ReadArray(Out, 4u, [](FMemoryImageReader& Element, FMaterialUniformPreshaderHeader& Header) {
        Header.OpcodeOffset = Element.Read<uint32>();
        Header.OpcodeSize = Element.Read<uint32>();
    });
}

void AddPreshadersUE4(FUniformExpressionSet& Value, const TArray<FMaterialUniformPreshaderHeader>& Headers, uint8 Type, uint32 NumComponents,
                      uint32& BufferOffset) {
    for (FMaterialUniformPreshaderHeader Header : Headers) {
        Header.FieldIndex = static_cast<uint32>(Value.UniformPreshaderFields.Num());
        Header.NumFields = 1u;
        Value.UniformPreshaderFields.Add({BufferOffset, 0u, Type});
        Value.UniformPreshaders.Add(Header);
        BufferOffset += NumComponents;
    }
}

template <class TDefaultValue>
void ReadNumericParametersUE4(FMemoryImageReader& Reader, EMaterialParameterType Type, FUniformExpressionSet& Value) {
    TArray<FMaterialNumericParameterInfo> Parameters;
    Reader.ReadArray(Parameters, GetParameterAlignment(Reader), [&](FMemoryImageReader& Element, FMaterialNumericParameterInfo& Parameter) {
        Parameter.ParameterInfo = ReadParameterInfo(Element);
        Parameter.ParameterType = Type;
        const TDefaultValue DefaultValue = Element.Read<TDefaultValue>();
        Parameter.DefaultValueOffset = static_cast<uint32>(Value.DefaultValues.AddUninitialized(sizeof(TDefaultValue)));
        std::memcpy(Value.DefaultValues.GetData() + Parameter.DefaultValueOffset, &DefaultValue, sizeof(TDefaultValue));
    });
    for (const FMaterialNumericParameterInfo& Parameter : Parameters)
        Value.UniformNumericParameters.Add(Parameter);
}

//! UE 4 fills the preshader buffer with the vector results first and packs the scalar results after them.
void UnfreezeUE4(FMemoryImageReader& Reader, FUniformExpressionSet& Value) {
    TArray<FMaterialUniformPreshaderHeader> VectorPreshaders;
    TArray<FMaterialUniformPreshaderHeader> ScalarPreshaders;
    ReadPreshaderHeadersUE4(Reader, VectorPreshaders);
    ReadPreshaderHeadersUE4(Reader, ScalarPreshaders);
    if (Reader.GetGame() == EGame::GAME_TheDivisionResurgence)
        Reader.Skip(32);
    uint32 BufferOffset = 0u;
    AddPreshadersUE4(Value, VectorPreshaders, ValueTypeFloat4, 4u, BufferOffset);
    AddPreshadersUE4(Value, ScalarPreshaders, ValueTypeFloat1, 1u, BufferOffset);
    Value.UniformPreshaderBufferSize = (BufferOffset + 3u) / 4u;
    ReadNumericParametersUE4<float>(Reader, EMaterialParameterType::Scalar, Value);
    ReadNumericParametersUE4<FLinearColor>(Reader, EMaterialParameterType::Vector, Value);
    const uint32 Alignment = GetParameterAlignment(Reader);
    for (EMaterialTextureParameterType Type : TextureParameterTypesUE4)
        Reader.ReadArray(Value.UniformTextureParameters[static_cast<uint32>(Type)], Alignment,
                         [](FMemoryImageReader& Element, FMaterialTextureParameterInfo& Parameter) { Unfreeze(Element, Parameter); });
    Reader.ReadArray(Value.UniformExternalTextureParameters, Alignment,
                     [](FMemoryImageReader& Element, FMaterialExternalTextureParameterInfo& Parameter) { Unfreeze(Element, Parameter); });
    Reader.ReadStruct(Value.UniformPreshaderData);
    Reader.ReadArray(Value.VTStacks);
    Reader.ReadArray(Value.ParameterCollections);
    if (Reader.GetGame() == EGame::GAME_HogwartsLegacy)
        Reader.Skip(168);
    Reader.ReadStruct(Value.UniformBufferLayoutInitializer);
}

}

void Unfreeze(FMemoryImageReader& Reader, FUniformExpressionSet& Value) {
    const EGame Game = Reader.GetGame();
    if (Game < EGame::GAME_UE5_0) {
        UnfreezeUE4(Reader, Value);
        return;
    }
    if (Game >= EGame::GAME_UE5_6)
        Reader.ReadArray(Value.UniformParameterEvaluations);
    //! Aion 2 writes a second array of numeric parameters before the preshaders.
    if (Game == EGame::GAME_Aion2) {
        TArray<FMaterialNumericParameterInfo> Unused;
        Reader.ReadArray(Unused);
    }
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
    } else if (Game >= EGame::GAME_UE5_1) {
        Reader.ReadArray(Value.UniformPreshaders);
        Reader.ReadArray(Value.UniformPreshaderFields);
    } else {
        Reader.ReadArray(Value.UniformPreshaders, 4u, [&](FMemoryImageReader& Element, FMaterialUniformPreshaderHeader& Header) {
            Header.OpcodeOffset = Element.Read<uint32>();
            Header.OpcodeSize = Element.Read<uint32>();
            const uint32 BufferOffset = Element.Read<uint32>();
            const uint8 ComponentType = Element.Read<uint8>();
            const uint8 NumComponents = Element.Read<uint8>();
            Header.FieldIndex = static_cast<uint32>(Value.UniformPreshaderFields.Num());
            Header.NumFields = 1u;
            Value.UniformPreshaderFields.Add({BufferOffset, 0u, GetValueTypeUE5_0(Element, ComponentType, NumComponents)});
        });
    }
    Reader.ReadArray(Value.UniformNumericParameters);
    if (Game == EGame::GAME_FateTrigger)
        Reader.Skip(16);
    const uint32 NumTextureParameterTypes = Game >= EGame::GAME_UE5_3 ? NumMaterialTextureParameterTypes : NumMaterialTextureParameterTypesUE5_0;
    for (uint32 Type = 0u; Type < NumTextureParameterTypes; Type++)
        Reader.ReadArray(Value.UniformTextureParameters[Type]);
    //! Infinity Nikki writes one more texture parameter type after the stock ones.
    if (Game == EGame::GAME_InfinityNikki)
        Reader.SkipArray();
    Reader.ReadArray(Value.UniformExternalTextureParameters);
    if (Game >= EGame::GAME_UE5_5 && Game != EGame::GAME_FateTrigger)
        Reader.ReadArray(Value.UniformTextureCollectionParameters);
    Value.UniformPreshaderBufferSize = Reader.Read<uint32>();
    if (Game >= EGame::GAME_UE5_9) {
        Value.UniformPreshaderHalfHead = Reader.Read<uint16>();
        Value.UniformPreshaderHalfTail = Reader.Read<uint16>();
    }
    Reader.ReadStruct(Value.UniformPreshaderData);
    Reader.ReadArray(Value.DefaultValues);
    Reader.ReadArray(Value.VTStacks);
    if (Game >= EGame::GAME_UE5_7 || Game == EGame::GAME_FateTrigger)
        Reader.ReadArray(Value.MaterialCacheTagStacks);
    Reader.ReadArray(Value.ParameterCollections);
    Reader.ReadStruct(Value.UniformBufferLayoutInitializer);
    if (Game >= EGame::GAME_UE5_8)
        Reader.SkipArray();
}

namespace {

using FCompilationFlag = bool FMaterialCompilationOutput::*;
using O = FMaterialCompilationOutput;

constexpr FCompilationFlag CompilationFlagsUE4_25[] = {
    &O::bNeedsSceneTextures,      &O::bUsesEyeAdaptation,    &O::bModifiesMeshPosition, &O::bUsesWorldPositionOffset,
    &O::bUsesGlobalDistanceField, &O::bUsesPixelDepthOffset, &O::bUsesDistanceCullFade, &O::bHasRuntimeVirtualTextureOutputNode};
constexpr FCompilationFlag CompilationFlagsUE4_26[] = {
    &O::bNeedsSceneTextures,      &O::bUsesEyeAdaptation,    &O::bModifiesMeshPosition, &O::bUsesWorldPositionOffset,
    &O::bUsesGlobalDistanceField, &O::bUsesPixelDepthOffset, &O::bUsesDistanceCullFade, &O::bHasRuntimeVirtualTextureOutputNode,
    &O::bUsesAnisotropy};
constexpr FCompilationFlag CompilationFlagsUE5_0[] = {&O::bNeedsSceneTextures,
                                                      &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,
                                                      &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,
                                                      &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesPerInstanceRandom,
                                                      &O::bUsesVertexInterpolator,
                                                      &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy};
constexpr FCompilationFlag CompilationFlagsUE5_3[] = {&O::bNeedsSceneTextures,    &O::bUsesDBufferTextureLookup, &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,  &O::bUsesWorldPositionOffset,  &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,  &O::bUsesDistanceCullFade,     &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesPerInstanceRandom, &O::bUsesVertexInterpolator,   &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy,        &O::bUsesDisplacement};
constexpr FCompilationFlag CompilationFlagsUE5_4[] = {&O::bNeedsSceneTextures,
                                                      &O::bUsesDBufferTextureLookup,
                                                      &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,
                                                      &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,
                                                      &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesVertexInterpolator,
                                                      &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy,
                                                      &O::bUsesDisplacement,
                                                      &O::bUsedWithNeuralNetworks};
constexpr FCompilationFlag CompilationFlagsUE5_5[] = {&O::bNeedsSceneTextures,
                                                      &O::bUsesDBufferTextureLookup,
                                                      &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,
                                                      &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,
                                                      &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesVertexInterpolator,
                                                      &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy,
                                                      &O::bIsLightFunctionAtlasCompatible,
                                                      &O::bUsesDisplacement,
                                                      &O::bUsedWithNeuralNetworks,
                                                      &O::bUsesCustomizedUVs};
constexpr FCompilationFlag CompilationFlagsUE5_7[] = {&O::bNeedsSceneTextures,
                                                      &O::bUsesDBufferTextureLookup,
                                                      &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,
                                                      &O::bUsesTemporalResponsiveness,
                                                      &O::bUsesMotionVectorWorldOffset,
                                                      &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,
                                                      &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesVertexInterpolator,
                                                      &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy,
                                                      &O::bIsLightFunctionAtlasCompatible,
                                                      &O::bUsesDisplacement,
                                                      &O::bUsedWithNeuralNetworks,
                                                      &O::bUsesCustomizedUVs};
constexpr FCompilationFlag CompilationFlagsUE5_8[] = {&O::bNeedsSceneTextures,
                                                      &O::bUsesDBufferTextureLookup,
                                                      &O::bUsesEyeAdaptation,
                                                      &O::bModifiesMeshPosition,
                                                      &O::bUsesWorldPositionOffset,
                                                      &O::bUsesTemporalResponsiveness,
                                                      &O::bUsesMotionVectorWorldOffset,
                                                      &O::bUsesGlobalDistanceField,
                                                      &O::bUsesPixelDepthOffset,
                                                      &O::bUsesDistanceCullFade,
                                                      &O::bUsesPerInstanceCustomData,
                                                      &O::bUsesVertexInterpolator,
                                                      &O::bHasRuntimeVirtualTextureOutputNode,
                                                      &O::bUsesAnisotropy,
                                                      &O::bIsLightFunctionAtlasCompatible,
                                                      &O::bUsesDisplacement,
                                                      &O::bUsedWithNeuralNetworks,
                                                      &O::bUsesCustomizedUVs,
                                                      &O::bUsesOpacityMask,
                                                      &O::bUsesCustomHLSLPixelDiscard,
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
    if (Game >= EGame::GAME_UE5_0)
        return CompilationFlagsUE5_0;
    if (Game >= EGame::GAME_UE4_26)
        return CompilationFlagsUE4_26;
    return CompilationFlagsUE4_25;
}

uint8 ReadCompilationFlags(FMemoryImageReader& Reader, FMaterialCompilationOutput& Value) {
    const std::span<const FCompilationFlag> Flags = GetCompilationFlags(Reader.GetGame());
    uint8 Bits = 0u;
    for (std::size_t Index = 0; Index < Flags.size(); Index++) {
        if (Index % 8u == 0u)
            Bits = Reader.Read<uint8>();
        Value.*Flags[Index] = (Bits & (1u << (Index % 8u))) != 0u;
    }
    return Bits;
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
    if (Game >= EGame::GAME_UE5_0)
        Value.UsedDBufferTextures = Reader.Read<uint8>();
    Value.RuntimeVirtualTextureOutputAttributeMask = Reader.Read<uint8>();
    const uint8 LastFlagBits = ReadCompilationFlags(Reader, Value);
    if (Game == EGame::GAME_UE5_2) {
        //! UE 5.2 packs the 2-bit StrataMaterialType into bits 4-5 of the last flag byte; the 3-bit StrataBSDFCount does not fit and starts a new byte.
        Value.StrataMaterialCompilationOutput.StrataMaterialType = (LastFlagBits >> 4u) & 3u;
        Value.StrataMaterialCompilationOutput.StrataBSDFCount = Reader.Read<uint8>() & 7u;
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
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_OnePieceAmbition)
        Ar.SkipBytes(20);
    int32 Quality = 0;
    int32 Feature = 0;
    if (Game >= EGame::GAME_UE5_2)
        Quality = Ar.Read<uint8>();
    else
        Ar << Quality;
    Ar << Feature;
    if (Game == EGame::GAME_ArenaBreakoutInfinite)
        Ar.SkipBytes(4);
    //! Roco Kingdom World swaps the quality and the feature level.
    if (Game == EGame::GAME_RocoKingdomWorld) {
        std::swap(Quality, Feature);
        Ar.SkipBytes(16);
    }
    QualityLevel = static_cast<EMaterialQualityLevel>(Quality);
    FeatureLevel = static_cast<ERHIFeatureLevel::Type>(Feature);
    if (Game == EGame::GAME_TheFirstDescendant)
        Ar.SkipBytes(4);
    if (Game >= EGame::GAME_UE5_8)
        Ar << CookedShaderMapIdHash.emplace<uint64>();
    else
        Ar << CookedShaderMapIdHash.emplace<FSHAHash>();
    Ar << LayoutParams;
}

namespace {

constexpr int32 SerializedNameSize = static_cast<int32>(sizeof(uint32) * 2);
constexpr int32 SerializedBoolSize = static_cast<int32>(sizeof(uint32));

void SkipLegacyArray(FArchive& Ar, int32 ElementSize) {
    int32 Count = Ar.ReadCount(ElementSize);
    Ar.SkipBytes(static_cast<int64>(Count) * ElementSize);
}

void SkipLegacyName(FArchive& Ar) {
    FName Name;
    Ar << Name;
}

void SkipLegacyParameterInfo(FArchive& Ar) {
    SkipLegacyName(Ar);
    Ar.Read<uint8>();
    Ar.Read<int32>();
}

void SkipLegacyStaticParameterName(FArchive& Ar) {
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MaterialAttributeLayerParameters, EGame::GAME_UE4_19))
        SkipLegacyParameterInfo(Ar);
    else
        SkipLegacyName(Ar);
}

void SkipLegacyStaticParameters(FArchive& Ar, int32 NumBools, int32 NumInts) {
    int32 NumParameters = Ar.ReadCount(SerializedNameSize);
    const int64 ValueSize = static_cast<int64>(NumBools + 1) * SerializedBoolSize + static_cast<int64>(NumInts) * 4 + static_cast<int64>(sizeof(FGuid));
    for (int32 Index = 0; Index < NumParameters && !Ar.IsError(); Index++) {
        SkipLegacyStaticParameterName(Ar);
        Ar.SkipBytes(ValueSize);
    }
}

void SkipLegacyTypeDependencies(FArchive& Ar, int32 ExtraSize) {
    SkipLegacyArray(Ar, SerializedNameSize + static_cast<int32>(sizeof(FSHAHash)) + ExtraSize);
}

void ReadLegacyCompilationFlags(FArchive& Ar, FMaterialCompilationOutput& Value) {
    const EGame Game = Ar.GetGame();
    bool bUnused = false;
    if (Game >= EGame::GAME_UE4_20) {
        Ar.SkipBytes(2 * static_cast<int64>(sizeof(uint8)) + 2 * static_cast<int64>(sizeof(uint16)));
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::PURGED_FMATERIAL_COMPILE_OUTPUTS)
        Ar << bUnused;
    Ar << Value.bNeedsSceneTextures << Value.bUsesEyeAdaptation << Value.bModifiesMeshPosition;
    if (Game >= EGame::GAME_UE4_13)
        Ar << Value.bUsesWorldPositionOffset;
    Ar << bUnused;
    if (Game >= EGame::GAME_UE4_9)
        Ar << Value.bUsesGlobalDistanceField;
    if (Game >= EGame::GAME_UE4_12)
        Ar << Value.bUsesPixelDepthOffset;
    if (Game >= EGame::GAME_UE4_14)
        Ar << bUnused;
    if (Game >= EGame::GAME_UE4_22)
        Ar << bUnused << Value.bUsesDistanceCullFade;
}

void ReadLegacyPackedCompilationFlags(FArchive& Ar, FMaterialCompilationOutput& Value) {
    Value.UsedSceneTextures = Ar.Read<uint32>();
    Ar.SkipBytes(3 * static_cast<int64>(sizeof(uint16)) + 2 * static_cast<int64>(sizeof(uint8)));
    const uint8 PackedFlags = Ar.Read<uint8>();
    Value.bUsesEyeAdaptation = (PackedFlags & 0x01u) != 0;
    Value.bModifiesMeshPosition = (PackedFlags & 0x02u) != 0;
    Value.bUsesWorldPositionOffset = (PackedFlags & 0x04u) != 0;
    Value.bUsesGlobalDistanceField = (PackedFlags & 0x08u) != 0;
    Value.bUsesPixelDepthOffset = (PackedFlags & 0x10u) != 0;
    Value.bUsesDistanceCullFade = (PackedFlags & 0x20u) != 0;
    Value.bHasRuntimeVirtualTextureOutputNode = (PackedFlags & 0x40u) != 0;
}

void SerializeLegacyCompilationOutput(FArchive& Ar, FMaterialCompilationOutput& Value) {
    SerializeLegacyUniformExpressionSet(Ar, Value.UniformExpressionSet);
    if (Ar.GetGame() >= EGame::GAME_UE4_23)
        ReadLegacyPackedCompilationFlags(Ar, Value);
    else
        ReadLegacyCompilationFlags(Ar, Value);
}

}

void SkipLegacyStaticParameterSet(FArchive& Ar) {
    SkipLegacyStaticParameters(Ar, 1, 0);
    SkipLegacyStaticParameters(Ar, 4, 0);
    const bool bWeightBasedBlend =
        Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::StaticParameterTerrainLayerWeightBlendType, EGame::GAME_UE4_20);
    SkipLegacyStaticParameters(Ar, bWeightBasedBlend ? 1 : 0, 1);
    if (!Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::MaterialLayersParameterSerializationRefactor, EGame::GAME_UE4_19))
        return;
    int32 NumLayers = Ar.ReadCount(SerializedNameSize);
    for (int32 Index = 0; Index < NumLayers && !Ar.IsError(); Index++) {
        SkipLegacyParameterInfo(Ar);
        Ar.SkipBytes(SerializedBoolSize + static_cast<int64>(sizeof(FGuid)));
        //! UE 4.22 stops writing the layer key string in cooked packages.
        if (Ar.GetGame() >= EGame::GAME_UE4_22)
            continue;
        FString KeyString;
        Ar << KeyString;
    }
}

void FMaterialShaderMapId::SerializeLegacy(FArchive& Ar) {
    int32 Quality = static_cast<int32>(EMaterialQualityLevel::Num);
    int32 Feature = static_cast<int32>(ERHIFeatureLevel::SM5);
    const bool bQualityAndFeature = Ar.UEVer() >= EUnrealEngineObjectUE4Version::PURGED_FMATERIAL_COMPILE_OUTPUTS;
    //! UE 4.23 cooks only the quality level, the feature level and a hash of the full shader map id.
    if (Ar.GetGame() >= EGame::GAME_UE4_23) {
        Ar << Quality << Feature;
        Ar << CookedShaderMapIdHash.emplace<FSHAHash>();
        QualityLevel = static_cast<EMaterialQualityLevel>(Quality);
        FeatureLevel = static_cast<ERHIFeatureLevel::Type>(Feature);
        return;
    }
    Ar.Read<uint32>();
    FGuid BaseMaterialId;
    Ar << BaseMaterialId;
    if (bQualityAndFeature)
        Ar << Quality << Feature;
    else
        Ar.Read<uint8>();
    QualityLevel = static_cast<EMaterialQualityLevel>(Quality);
    FeatureLevel = static_cast<ERHIFeatureLevel::Type>(Feature);
    SkipLegacyStaticParameterSet(Ar);
    constexpr int32 GuidSize = static_cast<int32>(sizeof(FGuid));
    SkipLegacyArray(Ar, GuidSize);
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::COLLECTIONS_IN_SHADERMAPID)
        SkipLegacyArray(Ar, GuidSize);
    if (Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::AddedMaterialSharedInputs, EGame::GAME_UE4_19)
        && !Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::RemovedMaterialSharedInputCollection, EGame::GAME_UE4_20))
        SkipLegacyArray(Ar, GuidSize);
    const bool bPermutationIds = Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::ShaderPermutationId, EGame::GAME_UE4_19);
    SkipLegacyTypeDependencies(Ar, bPermutationIds ? static_cast<int32>(sizeof(int32)) : 0);
    if (Ar.GetGame() >= EGame::GAME_UE4_11 && bQualityAndFeature)
        SkipLegacyTypeDependencies(Ar, 0);
    SkipLegacyTypeDependencies(Ar, 0);
    FSHAHash Hash;
    Ar << Hash;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::MATERIAL_INSTANCE_BASE_PROPERTY_OVERRIDES)
        Ar << Hash;
}

void FMaterialShaderMap::SerializeLegacy(FArchive& Ar) {
    ShaderMapId.SerializeLegacy(Ar);
    Content.ShaderPlatform = static_cast<uint8>(Ar.Read<int32>());
    ShaderPlatform = Content.ShaderPlatform;
    ShaderPlatformName = GetLegacyShaderPlatformName(Ar.GetGame(), ShaderPlatform);
    FString FriendlyName;
    Ar << FriendlyName;
    SerializeLegacyCompilationOutput(Ar, Content.MaterialCompilationOutput);
    FString DebugDescription;
    Ar << DebugDescription;
    SerializeLegacyShaderMapContent(Ar, Content, PointerTable, Code, bShareCode);
    int32 NumMeshShaderMaps = Ar.ReadCount(SerializedNameSize);
    for (int32 Index = 0; Index < NumMeshShaderMaps && !Ar.IsError(); Index++) {
        FMeshMaterialShaderMap& MeshShaderMap = Content.OrderedMeshShaderMaps[Content.OrderedMeshShaderMaps.AddDefaulted()];
        FName VertexFactoryType;
        Ar << VertexFactoryType;
        MeshShaderMap.VertexFactoryTypeName = FHashedName(VertexFactoryType);
        SerializeLegacyShaderMapContent(Ar, MeshShaderMap, PointerTable, Code, bShareCode);
    }
    if (Ar.GetGame() < EGame::GAME_UE4_20)
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (!Ar.IsError() && !bCooked)
        Ar.SetError(ErrorCode::InvalidData, "FMaterialShaderMap.bCooked");
}

void FMaterialShaderMap::Serialize(FArchive& Ar) {
    if (Ar.GetGame() < EGame::GAME_UE4_25) {
        SerializeLegacy(Ar);
        return;
    }
    //! UE 5.9 writes the shader map id after the shader map content, because the id hash includes the content hash.
    if (Ar.GetGame() >= EGame::GAME_UE5_9) {
        TShaderMap::Serialize(Ar);
        ShaderMapId.Serialize(Ar);
        return;
    }
    ShaderMapId.Serialize(Ar);
    //! UE 4 writes the layout of the frozen shader map only in the shader map id.
    if (Ar.GetGame() < EGame::GAME_UE5_0)
        LayoutParameters = ShaderMapId.LayoutParams;
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
    if (Ar.GetGame() == EGame::GAME_Stalker2)
        Ar.SkipBytes(8);
}

namespace {

class FMaterialResourceProxyReader : public FArchive {
public:
    explicit FMaterialResourceProxyReader(FArchive& InInnerArchive) : InnerArchive(InInnerArchive) {
        CopyStateFrom(InInnerArchive);
        Initialize();
        OffsetToFirstResource = InnerArchive.Tell();
    }

    void Serialize(void* Data, int64 Length) override {
        InnerArchive.Serialize(Data, Length);
        TakeInnerError();
    }
    int64 Tell() const override { return InnerArchive.Tell(); }
    int64 GetCookedPosition() const override { return InnerArchive.Tell() - OffsetToFirstResource; }
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
        if (GetGame() == EGame::GAME_ArenaBreakoutInfinite)
            SkipBytes(NumLocs);
        if (GetGame() == EGame::GAME_RocoKingdomWorld)
            SkipBytes(static_cast<int64>(NumLocs) * 5);
        Read<int32>();
    }

    FArchive& InnerArchive;
    TArray<FName> Names;
    int64 OffsetToFirstResource = 0;
};

}

namespace {

void SerializeProxiedShaderMaps(FArchive& Ar, FArchive& Source, int32 NumLoadedResources, TArray<FMaterialResource>& OutLoadedResources) {
    FMaterialResourceProxyReader ResourceAr(Source);
    for (int32 ResourceIndex = 0; ResourceIndex < NumLoadedResources && !ResourceAr.IsError(); ++ResourceIndex)
        OutLoadedResources[OutLoadedResources.AddDefaulted()].SerializeInlineShaderMap(ResourceAr);
    if (ResourceAr.IsError())
        Ar.SetError(ResourceAr.GetError().code, ResourceAr.GetError().where);
}

}

void SerializeInlineShaderMaps(FArchive& Ar, TArray<FMaterialResource>& OutLoadedResources) {
    int32 NumLoadedResources = Ar.ReadCount(8);
    OutLoadedResources.Empty(NumLoadedResources);
    if (Ar.IsError() || NumLoadedResources == 0)
        return;
    //! UE 4.21 moves the names and the shader map offsets of the material resources into a header before them.
    if (Ar.GetGame() < EGame::GAME_UE4_21) {
        for (int32 ResourceIndex = 0; ResourceIndex < NumLoadedResources && !Ar.IsError(); ++ResourceIndex)
            OutLoadedResources[OutLoadedResources.AddDefaulted()].SerializeInlineShaderMap(Ar);
        return;
    }
    if (Ar.GetGame() != EGame::GAME_Stalker2) {
        SerializeProxiedShaderMaps(Ar, Ar, NumLoadedResources, OutLoadedResources);
        return;
    }
    //! Stalker 2 moves the material resources into bulk data.
    FByteBulkData ShaderMaps;
    ShaderMaps.Serialize(Ar);
    if (Ar.IsError())
        return;
    Result<std::vector<std::byte>> Bytes = ShaderMaps.Read();
    if (!Bytes) {
        Ar.SetError(Bytes.error().code, Bytes.error().where);
        return;
    }
    FMemoryReaderView BulkAr(*Bytes);
    BulkAr.CopyStateFrom(Ar);
    SerializeProxiedShaderMaps(Ar, BulkAr, NumLoadedResources, OutLoadedResources);
}

}
