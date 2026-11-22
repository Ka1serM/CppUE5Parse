#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void USoundNode::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    //! Older packages keep the graph node only in editor builds, which cooked data never targets.
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT)
        return;
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsEditorDataStripped())
        return;
    UObject* Node = nullptr;
    Ar << Node;
    GraphNode = reinterpret_cast<UEdGraphNode*>(Node);
}

}
