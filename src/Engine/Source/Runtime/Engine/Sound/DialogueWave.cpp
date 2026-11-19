#include "Engine/Source/Runtime/Engine/Sound/DialogueWave.h"

namespace ue5parse::runtime {

void UDialogueWave::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    bool bCooked = false;
    Ar << bCooked;
}

}
