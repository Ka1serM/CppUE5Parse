#include "Engine/Source/Runtime/Engine/Sound/SoundCue.h"

#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void USoundCue::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    //! Older packages keep the graph only in editor builds, which cooked data never targets.
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT)
        return;
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsEditorDataStripped())
        return;
    UObject* Graph = nullptr;
    Ar << Graph;
    SoundCueGraph = reinterpret_cast<UEdGraph*>(Graph);
}

}
