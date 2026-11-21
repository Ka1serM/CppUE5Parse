#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void USoundNode::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() < EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT)
        return;
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Ar << GraphNode;
}

}
