#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

#include "SoundWaveProcedural.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundWaveProcedural : public USoundWave {
    GENERATED_BODY()
public:
    explicit USoundWaveProcedural(const FObjectInitializer& ObjectInitializer);

    void Serialize(FArchive& Ar) override;
};

}
