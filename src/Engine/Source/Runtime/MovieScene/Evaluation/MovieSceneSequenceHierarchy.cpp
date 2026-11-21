#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSequenceHierarchy.h"

namespace ue5parse::runtime {

//! 4.27 to 5.4 store the warp counter, which 5.5 dropped.
FArchive& operator<<(FArchive& Ar, FMovieSceneSubSequenceTreeEntry& Entry) {
    Ar << Entry.SequenceID << Entry.Flags;
    if (Ar.GetGame() >= EGame::GAME_UE5_5)
        return Ar;
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::AddedSubSequenceEntryWarpCounter, EGame::GAME_UE4_27)
        || Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::AddedSubSequenceEntryWarpCounter, EGame::GAME_UE5_0))
        FMovieSceneWarpCounter::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&Entry.RootToSequenceWarpCounter));
    return Ar;
}

}
