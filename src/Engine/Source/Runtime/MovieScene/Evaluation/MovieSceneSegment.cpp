#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSegment.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FSectionEvaluationData& Data) {
    FSectionEvaluationData::StaticStruct()->SerializeItem(Ar, &Data);
    return Ar;
}

bool FSectionEvaluationData::Serialize(FArchive& Ar) {
    if (Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::FloatToIntConversion, EGame::GAME_UE4_20))
        return false;
    FLegacySectionEvaluationData Legacy;
    FLegacySectionEvaluationData::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&Legacy));
    ImplIndex = Legacy.ImplIndex;
    Flags = Legacy.Flags;
    ForcedTime = Legacy.ForcedTime == std::numeric_limits<float>::lowest() ? FFrameNumber{std::numeric_limits<int32>::lowest()}
                                                                          : UpgradeLegacyMovieSceneTime(GetLegacyConversionFrameRate(), Legacy.ForcedTime);
    return true;
}

bool FMovieSceneSegment::Serialize(FArchive& Ar) {
    SerializeMovieSceneFrameNumberRange(Ar, Range);
    if (Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::EvaluationTree, EGame::GAME_UE4_19))
        Ar << ID << bAllowEmpty;
    int32 NumStructs = 0;
    Ar << NumStructs;
    for (int32 Index = 0; Index < NumStructs && !Ar.IsError(); ++Index)
        Ar << Impls.Emplace_GetRef();
    return true;
}

}
