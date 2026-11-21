#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationField.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTrack.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSequenceHierarchy.h"

#include "MovieSceneEvaluationTemplate.generated.h"

namespace ue5parse::runtime {

class UMovieSceneSubSection;

USTRUCT()
struct FMovieSceneEvaluationTemplateSerialNumber {
    GENERATED_BODY()

    UPROPERTY()
    uint32 Value = 0;
};

USTRUCT()
struct FMovieSceneTrackFieldData {
    GENERATED_BODY()

    TMovieSceneEvaluationTree<FMovieSceneTrackIdentifier> Field;

    bool Serialize(FArchive& Ar) {
        Ar << Field;
        return true;
    }
    bool operator==(const FMovieSceneTrackFieldData&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTrackFieldData> : TStructOpsTypeTraitsBase2<FMovieSceneTrackFieldData> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneSubSectionData {
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UMovieSceneSubSection> Section;
    UPROPERTY()
    FGuid ObjectBindingId;
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;

    bool operator==(const FMovieSceneSubSectionData&) const = default;
};
FArchive& operator<<(FArchive& Ar, FMovieSceneSubSectionData& Data);

USTRUCT()
struct FMovieSceneSubSectionFieldData {
    GENERATED_BODY()

    TMovieSceneEvaluationTree<FMovieSceneSubSectionData> Field;

    bool Serialize(FArchive& Ar) {
        Ar << Field;
        return true;
    }
    bool operator==(const FMovieSceneSubSectionFieldData&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSubSectionFieldData> : TStructOpsTypeTraitsBase2<FMovieSceneSubSectionFieldData> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

//! The 4.16 to 4.18 ledger, which maps a signature to several tracks.
USTRUCT()
struct FLegacyMovieSceneTemplateGenerationLedger {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneTrackIdentifier LastTrackIdentifier;
    UPROPERTY()
    TMap<FMovieSceneTrackIdentifier, int32> TrackReferenceCounts;
    UPROPERTY()
    TMap<FGuid, FMovieSceneTrackIdentifiers> TrackSignatureToTrackIdentifier;
};

USTRUCT()
struct FMovieSceneTemplateGenerationLedger {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneTrackIdentifier LastTrackIdentifier;
    UPROPERTY()
    TMap<FGuid, FMovieSceneTrackIdentifier> TrackSignatureToTrackIdentifier;
    UPROPERTY()
    TMap<FGuid, FMovieSceneFrameRange> SubSectionRanges;
    UPROPERTY()
    FLegacyMovieSceneTemplateGenerationLedger LegacyLedger;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTemplateGenerationLedger> : TStructOpsTypeTraitsBase2<FMovieSceneTemplateGenerationLedger> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMovieSceneGenerationLedger {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneTrackIdentifier LastTrackIdentifier;
    TMap<FMovieSceneTrackIdentifier, int32> TrackReferenceCounts;
    TMap<FGuid, TArray<FMovieSceneTrackIdentifier>> TrackSignatureToTrackIdentifier;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneGenerationLedger> : TStructOpsTypeTraitsBase2<FMovieSceneGenerationLedger> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMovieSceneTrackCompilationParams {
    GENERATED_BODY()

    UPROPERTY()
    bool bForEditorPreview = false;
    UPROPERTY()
    bool bDuringBlueprintCompile = false;
};

USTRUCT()
struct FMovieSceneEvaluationTemplate {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack> Tracks;
    UPROPERTY()
    FMovieSceneEvaluationField EvaluationField;
    UPROPERTY()
    FMovieSceneSequenceHierarchy Hierarchy;
    UPROPERTY()
    FMovieSceneGenerationLedger Ledger;
    UPROPERTY()
    FMovieSceneTemplateGenerationLedger TemplateLedger;
    UPROPERTY()
    bool bHasLegacyTrackInstances = false;
    UPROPERTY()
    bool bKeepStaleTracks = false;
    UPROPERTY()
    FGuid SequenceSignature;
    UPROPERTY()
    FMovieSceneTrackFieldData TrackFieldData;
    UPROPERTY()
    FMovieSceneSubSectionFieldData SubSectionFieldData;
    UPROPERTY()
    FMovieSceneEvaluationTemplateSerialNumber TemplateSerialNumber;
};

USTRUCT()
struct FCachedMovieSceneEvaluationTemplate : public FMovieSceneEvaluationTemplate {
    GENERATED_BODY()
};

}
