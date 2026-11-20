#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

struct FNiagaraTypeDefinitionHandle {
    int32 RegisteredTypeIndex = INDEX_NONE;
    bool operator==(const FNiagaraTypeDefinitionHandle&) const = default;
};

struct FNiagaraVariableBase {
    FName Name;
    FNiagaraTypeDefinitionHandle TypeDefHandle;
    bool operator==(const FNiagaraVariableBase&) const = default;
};

struct FNiagaraVariable : FNiagaraVariableBase {
    TArray<uint8> VarData;
    bool operator==(const FNiagaraVariable&) const = default;
};

struct FNiagaraVariableWithOffset : FNiagaraVariableBase {
    int32 Offset = 0;
    bool operator==(const FNiagaraVariableWithOffset&) const = default;
};

}
