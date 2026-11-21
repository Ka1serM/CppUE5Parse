#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShared.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FNiagaraDataInterfaceGeneratedFunction::Serialize(FArchive& Ar) {
    Ar << DefinitionName << InstanceName << Specifiers;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddVariadicParametersToGPUFunctionInfo, EGame::GAME_UE5_2))
        Ar << VariadicInputs << VariadicOutputs;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::SerializeUsageBitMaskToGPUFunctionInfo, EGame::GAME_UE5_6))
        Ar << MiscUsageBitMask;
    return true;
}

bool FNiagaraDataInterfaceGPUParamInfo::Serialize(FArchive& Ar) {
    Ar << DataInterfaceHLSLSymbol << DIClassName;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddGeneratedFunctionsToGPUParamInfo, EGame::GAME_UE4_25))
        Ar << GeneratedFunctions;
    return true;
}

}
