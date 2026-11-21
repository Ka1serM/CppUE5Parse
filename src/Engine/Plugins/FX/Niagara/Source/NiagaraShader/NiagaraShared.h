#pragma once

#include "Engine/Plugins/FX/Niagara/Source/NiagaraCore/NiagaraCore.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

struct FNiagaraDataInterfaceGeneratedFunction {
    struct FunctionSpecifier {
        FName Key;
        FName Value;
        bool operator==(const FunctionSpecifier&) const = default;
        friend FArchive& operator<<(FArchive& Ar, FunctionSpecifier& Specifier) { return Ar << Specifier.Key << Specifier.Value; }
    };

    FName DefinitionName;
    FString InstanceName;
    TArray<FunctionSpecifier> Specifiers;
    uint16 MiscUsageBitMask = 0;
    TArray<FNiagaraVariableCommonReference> VariadicInputs;
    TArray<FNiagaraVariableCommonReference> VariadicOutputs;
    bool operator==(const FNiagaraDataInterfaceGeneratedFunction&) const = default;

    bool Serialize(FArchive& Ar) {
        Ar << DefinitionName << InstanceName << Specifiers;
        if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddVariadicParametersToGPUFunctionInfo, EGame::GAME_UE5_2))
            Ar << VariadicInputs << VariadicOutputs;
        if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::SerializeUsageBitMaskToGPUFunctionInfo, EGame::GAME_UE5_6))
            Ar << MiscUsageBitMask;
        return true;
    }

    friend FArchive& operator<<(FArchive& Ar, FNiagaraDataInterfaceGeneratedFunction& Function) {
        Function.Serialize(Ar);
        return Ar;
    }
};

struct FNiagaraDataInterfaceGPUParamInfo {
    FString DataInterfaceHLSLSymbol;
    FString DIClassName;
    uint32 ShaderParametersOffset = static_cast<uint32>(INDEX_NONE);
    TArray<FNiagaraDataInterfaceGeneratedFunction> GeneratedFunctions;
    bool operator==(const FNiagaraDataInterfaceGPUParamInfo&) const = default;

    bool Serialize(FArchive& Ar) {
        Ar << DataInterfaceHLSLSymbol << DIClassName;
        if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddGeneratedFunctionsToGPUParamInfo, EGame::GAME_UE4_25))
            Ar << GeneratedFunctions;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FNiagaraDataInterfaceGeneratedFunction> : TStructOpsTypeTraitsBase2<FNiagaraDataInterfaceGeneratedFunction> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

template <>
struct TStructOpsTypeTraits<FNiagaraDataInterfaceGPUParamInfo> : TStructOpsTypeTraitsBase2<FNiagaraDataInterfaceGPUParamInfo> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
