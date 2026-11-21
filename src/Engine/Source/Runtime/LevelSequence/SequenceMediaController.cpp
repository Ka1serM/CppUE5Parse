#include "Engine/Source/Runtime/LevelSequence/SequenceMediaController.h"

#include "Engine/Source/Runtime/MediaAssets/MediaComponent.h"

namespace ue5parse::runtime {

ALevelSequenceMediaController::ALevelSequenceMediaController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bReplicates = true;
    MediaComponent = ObjectInitializer.CreateDefaultSubobject<UMediaComponent>(FName("MediaComponent"));
}

}
