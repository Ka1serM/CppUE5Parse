#include "Engine/Source/Runtime/Engine/Sound/SoundWaveProcedural.h"

namespace ue5parse::runtime {

USoundWaveProcedural::USoundWaveProcedural(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bProcedural = true;
}

}
