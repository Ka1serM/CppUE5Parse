#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FRawAnimSequenceTrack::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RawAnimSequenceTrackSerializer, EGame::GAME_UE5_1))
        return false;
    BulkSerialize(Ar, PosKeys);
    BulkSerialize(Ar, RotKeys);
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::ANIM_SUPPORT_NONUNIFORM_SCALE_ANIMATION)
        BulkSerialize(Ar, ScaleKeys);
    return true;
}

}
