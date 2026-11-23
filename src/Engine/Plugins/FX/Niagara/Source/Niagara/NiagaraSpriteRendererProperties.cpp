#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSpriteRendererProperties.h"

namespace ue5parse::runtime {

void UNiagaraSpriteRendererProperties::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() < EGame::GAME_UE4_23)
        return;
    DerivedData.Serialize(Ar);
}

}
