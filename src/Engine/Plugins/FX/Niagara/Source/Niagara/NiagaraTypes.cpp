#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FNiagaraVariableBase::Serialize(FArchive& Ar) {
    //! Hell Let Loose saves its Niagara variables as tagged properties.
    if (Ar.GetGame() == EGame::GAME_HellLetLoose)
        return false;
    if (!Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::VariablesUseTypeDefRegistry, EGame::GAME_UE4_26))
        return false;
    Ar << Name;
    FNiagaraTypeDefinition::StaticStruct()->SerializeItem(Ar, &TypeDef);
    return true;
}

bool FNiagaraVariableBase::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("NiagaraVariable"))
        return false;
    FNiagaraVariable Variable;
    FNiagaraVariable::StaticStruct()->SerializeItem(Ar, &Variable);
    *this = Variable;
    return true;
}

bool FNiagaraVariable::Serialize(FArchive& Ar) {
    if (!FNiagaraVariableBase::Serialize(Ar))
        return false;
    if (Ar.GetGame() == EGame::GAME_FinalFantasy7Rebirth)
        Ar.SkipBytes(8);
    Ar << VarData;
    if (Ar.GetGame() == EGame::GAME_FinalFantasy7Rebirth)
        Ar.SkipBytes(4);
    return true;
}

bool FNiagaraVariableWithOffset::Serialize(FArchive& Ar) {
    if (!FNiagaraVariableBase::Serialize(Ar))
        return false;
    Ar << Offset;
    return true;
}

}
