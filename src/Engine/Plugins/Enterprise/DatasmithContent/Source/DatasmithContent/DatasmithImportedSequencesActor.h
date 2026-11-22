#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DatasmithImportedSequencesActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ADatasmithImportedSequencesActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADatasmithImportedSequencesActor(const FObjectInitializer& ObjectInitializer);
};

}
