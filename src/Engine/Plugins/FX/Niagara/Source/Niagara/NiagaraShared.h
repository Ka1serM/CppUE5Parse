#pragma once

#include <optional>

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCompileHash.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCore.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/RenderCore/Shader.h"

#include "NiagaraShared.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraDataInterfaceGeneratedFunction {
    GENERATED_BODY()

    FName DefinitionName;
    FString InstanceName;
    TArray<TPair<FName, FName>> Specifiers;
    uint16 MiscUsageBitMask = 0;
    UPROPERTY()
    TArray<FNiagaraVariableCommonReference> VariadicInputs;
    UPROPERTY()
    TArray<FNiagaraVariableCommonReference> VariadicOutputs;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FNiagaraDataInterfaceGeneratedFunction& Function) {
        Function.Serialize(Ar);
        return Ar;
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraDataInterfaceGeneratedFunction> : TStructOpsTypeTraitsBase2<FNiagaraDataInterfaceGeneratedFunction> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FNiagaraDataInterfaceGPUParamInfo {
    GENERATED_BODY()

    UPROPERTY()
    FString DataInterfaceHLSLSymbol;
    UPROPERTY()
    FString DIClassName;
    UPROPERTY()
    TArray<FNiagaraDataInterfaceGeneratedFunction> GeneratedFunctions;
    UPROPERTY()
    uint32 ShaderParametersOffset = INDEX_NONE;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FNiagaraDataInterfaceGPUParamInfo> : TStructOpsTypeTraitsBase2<FNiagaraDataInterfaceGPUParamInfo> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FNiagaraShaderScriptExternalConstant {
    GENERATED_BODY()

    UPROPERTY()
    FName Type;
    UPROPERTY()
    FString Name;
};

USTRUCT()
struct FNiagaraShaderScriptParametersMetadata {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraDataInterfaceGPUParamInfo> DataInterfaceParamInfo;
    UPROPERTY()
    TArray<FString> LooseMetadataNames;
    UPROPERTY()
    bool bExternalConstantsInterpolated = false;
    UPROPERTY()
    TArray<FNiagaraShaderScriptExternalConstant> ExternalConstants;
};

class FNiagaraShaderMapPointerTable : public FShaderMapPointerTable {
public:
    void LoadFromArchive(FArchive& Ar) override;

    TArray<FString> DITypes;
};

class FNiagaraShaderMapId {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    FGuid CompilerVersionID;
    ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::Num;
    FGuid BaseScriptID_DEPRECATED;
    TArray<FString> AdditionalDefines;
    TArray<FString> AdditionalVariables;
    FSHAHash BaseCompileHash;
    TArray<FSHAHash> ReferencedCompileHashes;
    FPlatformTypeLayoutParameters LayoutParams;
    TArray<FShaderTypeDependency> ShaderTypeDependencies;
    bool bUsesRapidIterationParams = true;
};

class FNiagaraShaderMapContent : public FShaderMapContent {
public:
    FString FriendlyName;
    FString DebugDescription;
    FNiagaraShaderMapId ShaderMapId;
};

class FNiagaraShaderMap : public TShaderMap<FNiagaraShaderMapContent, FNiagaraShaderMapPointerTable> {};

class FNiagaraShaderScript {
public:
    void SerializeShaderMap(FArchive& Ar);

    bool bLoadedFromCookedMaterial = false;
    int32 NumPermutations = 0;
    TArray<TPair<int32, int32>> ShaderStageToPermutation;
    FNiagaraCompileHash BaseCompileHash;
    std::optional<FNiagaraShaderMap> GameThreadShaderMap;
};

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapId& Value);
void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapContent& Value);

}
