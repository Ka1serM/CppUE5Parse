#include "Engine/Source/Runtime/Engine/Sound/SoundWaveProcedural.h"

namespace ue5parse::runtime {

USoundWaveProcedural::USoundWaveProcedural(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bProcedural = true;
}

void USoundWaveProcedural::Serialize(FArchive& Ar) {
    //! UE skips the USoundWave serializer, which would read the compressed audio.
    USoundBase::Serialize(Ar);
}

}
