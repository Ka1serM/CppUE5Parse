#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSystem.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UNiagaraSystem::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::ChangeEmitterCompiledDataToSharedRefs, EGame::GAME_UE4_24))
        return;
    const int32 Num = Ar.ReadCount();
    EmitterCompiledData.SetNum(Num);
    for (FNiagaraEmitterCompiledData& Data : EmitterCompiledData)
        FNiagaraEmitterCompiledData::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&Data));
}

}
