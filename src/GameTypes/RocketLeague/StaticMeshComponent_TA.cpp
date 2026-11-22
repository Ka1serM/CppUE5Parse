#include "GameTypes/RocketLeague/StaticMeshComponent_TA.h"

namespace ue5parse::runtime {

void UStaticMeshComponent_TA::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!Ar.IsError())
        Ar << bIgnoreScriptAttach;
}

}
