#include "Engine/Plugins/Enterprise/DatasmithContent/Source/DatasmithContent/DatasmithImportedSequencesActor.h"

namespace ue5parse::runtime {

ADatasmithImportedSequencesActor::ADatasmithImportedSequencesActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
