#include "Engine/Source/Runtime/Engine/Sound/SoundClass.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FSoundClassEditorData& EditorData) {
    return Ar << EditorData.NodePosX << EditorData.NodePosY;
}

void USoundClass::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() >= EUnrealEngineObjectUE4Version::SOUND_CLASS_GRAPH_EDITOR)
        return;
    const int32 Count = Ar.ReadCount(1);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        UObject* SoundClass = nullptr;
        FSoundClassEditorData EditorData;
        Ar << SoundClass << EditorData;
    }
}

}
