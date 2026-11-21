#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCompileHash.h"
#include "Engine/Source/Runtime/RenderCore/Shader.h"

namespace ue5parse::runtime {

struct FShaderTypeDependency {
    FHashedName ShaderTypeName;
    int32 PermutationId = 0;
    FSHAHash SourceHash;
};

struct FNiagaraShaderMapId {
    static constexpr uint32 FrozenAlignment = 8u;

    FGuid CompilerVersionID;
    int32 FeatureLevel = 0;
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

class FNiagaraShaderMapPointerTable : public FShaderMapPointerTable {
public:
    void LoadFromArchive(FArchive& Ar) override;
    TArray<FString> DITypes;
};

using FNiagaraShaderMap = TShaderMap<FNiagaraShaderMapContent, FNiagaraShaderMapPointerTable>;

struct FNiagaraShaderScript {
    bool bLoadedFromCookedMaterial = false;
    int32 NumPermutations = 0;
    FNiagaraCompileHash BaseCompileHash;
    std::optional<FNiagaraShaderMap> ShaderMap;

    friend FArchive& operator<<(FArchive& Ar, FNiagaraShaderScript& Script);
};

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapId& Id);
void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapContent& Content);

}
