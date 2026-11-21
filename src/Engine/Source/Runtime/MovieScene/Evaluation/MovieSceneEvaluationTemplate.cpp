#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTemplate.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FMovieSceneSubSectionData& Data) {
    UObject* Section = nullptr;
    Ar << Section;
    Data.Section = reinterpret_cast<UMovieSceneSubSection*>(Section);
    return Ar << Data.ObjectBindingId << Data.Flags;
}

bool FMovieSceneTemplateGenerationLedger::Serialize(FArchive& Ar) {
    if (Ar.GetGame() >= EGame::GAME_UE4_19)
        return false;
    FLegacyMovieSceneTemplateGenerationLedger::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&LegacyLedger));
    LastTrackIdentifier = LegacyLedger.LastTrackIdentifier;
    return true;
}

bool FMovieSceneGenerationLedger::Serialize(FArchive& Ar) {
    int32 NumReferenceCounts = 0;
    Ar << NumReferenceCounts;
    for (int32 Index = 0; Index < NumReferenceCounts && !Ar.IsError(); ++Index) {
        FMovieSceneTrackIdentifier Identifier;
        FMovieSceneTrackIdentifier::StaticStruct()->SerializeItem(Ar, &Identifier);
        int32 Count = 0;
        Ar << Count;
        TrackReferenceCounts.Add(Identifier, Count);
    }
    int32 NumSignatures = 0;
    Ar << NumSignatures;
    for (int32 Index = 0; Index < NumSignatures && !Ar.IsError(); ++Index) {
        FGuid Signature;
        Ar << Signature;
        TArray<FMovieSceneTrackIdentifier>& Identifiers = TrackSignatureToTrackIdentifier.Add(Signature);
        int32 NumIdentifiers = 0;
        Ar << NumIdentifiers;
        for (int32 IdentifierIndex = 0; IdentifierIndex < NumIdentifiers && !Ar.IsError(); ++IdentifierIndex)
            FMovieSceneTrackIdentifier::StaticStruct()->SerializeItem(Ar, &Identifiers.Emplace_GetRef());
    }
    return true;
}

}
