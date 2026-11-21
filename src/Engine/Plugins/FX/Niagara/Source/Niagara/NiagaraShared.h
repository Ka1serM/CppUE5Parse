#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCore.h"

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

}
