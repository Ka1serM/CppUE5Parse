#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FNiagaraVariableBase::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::VariablesUseTypeDefRegistry, EGame::GAME_UE4_26))
        return false;
    Ar << Name;
    FNiagaraTypeDefinition::StaticStruct()->SerializeItem(Ar, &TypeDef);
    return true;
}

bool FNiagaraVariable::Serialize(FArchive& Ar) {
    if (!FNiagaraVariableBase::Serialize(Ar))
        return false;
    Ar << VarData;
    return true;
}

bool FNiagaraVariableWithOffset::Serialize(FArchive& Ar) {
    if (!FNiagaraVariableBase::Serialize(Ar))
        return false;
    Ar << Offset;
    return true;
}

}
