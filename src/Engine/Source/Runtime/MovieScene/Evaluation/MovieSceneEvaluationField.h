#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationKey.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSegment.h"

#include "MovieSceneEvaluationField.generated.h"

namespace ue5parse::runtime {

class UMovieSceneCondition;

USTRUCT()
struct FMovieSceneEvaluationFieldEntityKey {
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UObject> EntityOwner;
    UPROPERTY()
    uint32 EntityID = 0;
};

USTRUCT()
struct FMovieSceneEvaluationFieldEntity {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneEvaluationFieldEntityKey Key;
    UPROPERTY()
    int32 SharedMetaDataIndex = -1;
};

USTRUCT()
struct FMovieSceneEvaluationFieldSharedEntityMetaData {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ObjectBindingID;
};

USTRUCT()
struct FMovieSceneEvaluationFieldEntityMetaData {
    GENERATED_BODY()

    UPROPERTY()
    FString OverrideBoundPropertyPath;
    UPROPERTY()
    FFrameNumber ForcedTime{std::numeric_limits<int32>::lowest()};
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;
    UPROPERTY()
    bool bEvaluateInSequencePreRoll = false;
    UPROPERTY()
    bool bEvaluateInSequencePostRoll = false;
    UPROPERTY()
    TSoftObjectPtr<UMovieSceneCondition> Condition;
    UPROPERTY()
    FName NotifyFunctionName;
};

struct FEntityAndMetaDataIndex {
    int32 EntityIndex = 0;
    int32 MetaDataIndex = 0;
    bool operator==(const FEntityAndMetaDataIndex&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FEntityAndMetaDataIndex& Index) {
    return Ar << Index.EntityIndex << Index.MetaDataIndex;
}

USTRUCT()
struct FMovieSceneEvaluationFieldEntityTree {
    GENERATED_BODY()

    TMovieSceneEvaluationTree<FEntityAndMetaDataIndex> SerializedData;

    bool Serialize(FArchive& Ar) {
        Ar << SerializedData;
        return true;
    }
    bool operator==(const FMovieSceneEvaluationFieldEntityTree&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEvaluationFieldEntityTree> : TStructOpsTypeTraitsBase2<FMovieSceneEvaluationFieldEntityTree> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneEntityComponentField {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneEvaluationFieldEntityTree PersistentEntityTree;
    UPROPERTY()
    FMovieSceneEvaluationFieldEntityTree OneShotEntityTree;
    UPROPERTY()
    TArray<FMovieSceneEvaluationFieldEntity> Entities;
    UPROPERTY()
    TArray<FMovieSceneEvaluationFieldEntityMetaData> EntityMetaData;
    UPROPERTY()
    TArray<FMovieSceneEvaluationFieldSharedEntityMetaData> SharedMetaData;
};

USTRUCT()
struct FMovieSceneEvaluationFieldTrackPtr {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneSequenceID SequenceID;
    UPROPERTY()
    FMovieSceneTrackIdentifier TrackIdentifier;
};

USTRUCT()
struct FMovieSceneEvaluationFieldSegmentPtr : public FMovieSceneEvaluationFieldTrackPtr {
    GENERATED_BODY()

    UPROPERTY()
    int32 SegmentIndex = 0;
    UPROPERTY()
    FMovieSceneSegmentIdentifier SegmentID;
};

USTRUCT()
struct FMovieSceneFieldEntry_EvaluationTrack {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneEvaluationFieldTrackPtr TrackPtr;
    UPROPERTY()
    uint16 NumChildren = 0;
};

USTRUCT()
struct FMovieSceneFieldEntry_ChildTemplate {
    GENERATED_BODY()

    UPROPERTY()
    uint16 ChildIndex = static_cast<uint16>(-1);
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;
    UPROPERTY()
    FFrameNumber ForcedTime{std::numeric_limits<int32>::lowest()};
};

USTRUCT()
struct FMovieSceneEvaluationGroupLUTIndex {
    GENERATED_BODY()

    UPROPERTY()
    bool bRequiresImmediateFlush = false;
    UPROPERTY()
    int32 LUTOffset = 0;
    UPROPERTY()
    int32 NumInitPtrs = 0;
    UPROPERTY()
    int32 NumEvalPtrs = 0;
};

USTRUCT()
struct FMovieSceneEvaluationGroup {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneEvaluationGroupLUTIndex> LUTIndices;
    UPROPERTY()
    TArray<FMovieSceneEvaluationFieldSegmentPtr> SegmentPtrLUT;
    UPROPERTY()
    TArray<FMovieSceneFieldEntry_EvaluationTrack> TrackLUT;
    UPROPERTY()
    TArray<FMovieSceneFieldEntry_ChildTemplate> SectionLUT;
};

USTRUCT()
struct FMovieSceneEvaluationMetaData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneSequenceID> ActiveSequences;
    UPROPERTY()
    TArray<FMovieSceneOrderedEvaluationKey> ActiveEntities;
    UPROPERTY()
    TMap<FMovieSceneSequenceID, FGuid> SubSequenceSignatures;
    UPROPERTY()
    TMap<FMovieSceneSequenceID, uint32> SubTemplateSerialNumbers;
};

USTRUCT()
struct FMovieSceneEvaluationField {
    GENERATED_BODY()

    UPROPERTY()
    FGuid Signature;
    UPROPERTY()
    TArray<FMovieSceneFrameRange> Ranges;
    UPROPERTY()
    TArray<FMovieSceneEvaluationGroup> Groups;
    UPROPERTY()
    TArray<FMovieSceneEvaluationMetaData> MetaData;
};

}
