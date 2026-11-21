#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvalTemplate.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSegment.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneSequenceTransform.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSequenceID.h"

#include "MovieSceneSequenceHierarchy.generated.h"

namespace ue5parse::runtime {

class UMovieSceneCondition;

UENUM()
enum class EMovieSceneSubSectionFlags : uint8 { None = 0, OverrideKeepState = 1 << 0, OverrideRestoreState = 1 << 1, IgnoreHierarchicalBias = 1 << 2, BlendHierarchicalBias = 1 << 3 };

UENUM()
enum class EMovieSceneServerClientMask : uint8 { None = 0, Server = 1 << 0, Client = 1 << 1, All = Server | Client };

USTRUCT()
struct FMovieSceneSubSequenceData {
    GENERATED_BODY()

    UPROPERTY()
    FSoftObjectPath Sequence;
    UPROPERTY()
    TObjectPtr<UObject> SequenceKeyObject = nullptr;
    UPROPERTY()
    FGuid SourceSequenceSignature;
    UPROPERTY()
    FMovieSceneSequenceTransform RootToSequenceTransform;
    UPROPERTY()
    FFrameRate TickResolution = {60000, 1};
    UPROPERTY()
    FMovieSceneSequenceID DeterministicSequenceID;
    UPROPERTY()
    FMovieSceneFrameRange PlayRange;
    UPROPERTY()
    FMovieSceneFrameRange FullPlayRange;
    UPROPERTY()
    FMovieSceneFrameRange UnwarpedPlayRange;
    UPROPERTY()
    FMovieSceneFrameRange ParentPlayRange;
    UPROPERTY()
    FFrameNumber ParentStartFrameOffset;
    UPROPERTY()
    FFrameNumber ParentEndFrameOffset;
    UPROPERTY()
    FFrameNumber ParentFirstLoopStartFrameOffset;
    UPROPERTY()
    bool bCanLoop = false;
    UPROPERTY()
    FMovieSceneFrameRange PreRollRange;
    UPROPERTY()
    FMovieSceneFrameRange PostRollRange;
    UPROPERTY()
    int16 HierarchicalBias = 0;
    UPROPERTY()
    EMovieSceneSubSectionFlags AccumulatedFlags = EMovieSceneSubSectionFlags::None;
    UPROPERTY()
    bool bHasHierarchicalEasing = false;
    UPROPERTY()
    FMovieSceneSequenceInstanceDataPtr InstanceData;
    UPROPERTY()
    FGuid SubSectionSignature;
    UPROPERTY()
    FMovieSceneSequenceTransform OuterToInnerTransform;
    UPROPERTY()
    FName SectionPath;
    UPROPERTY()
    TSoftObjectPtr<UMovieSceneCondition> WeakCondition;
};

USTRUCT()
struct FMovieSceneSequenceHierarchyNode {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneSequenceID ParentID;
    UPROPERTY()
    TArray<FMovieSceneSequenceID> Children;
};

USTRUCT()
struct FMovieSceneSubSequenceTreeEntry {
    GENERATED_BODY()

    FMovieSceneSequenceID SequenceID;
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;
    FMovieSceneWarpCounter RootToSequenceWarpCounter;

    bool operator==(const FMovieSceneSubSequenceTreeEntry&) const = default;
};
FArchive& operator<<(FArchive& Ar, FMovieSceneSubSequenceTreeEntry& Entry);

USTRUCT()
struct FMovieSceneSubSequenceTree {
    GENERATED_BODY()

    TMovieSceneEvaluationTree<FMovieSceneSubSequenceTreeEntry> Data;

    bool Serialize(FArchive& Ar) {
        Ar << Data;
        return true;
    }
    bool operator==(const FMovieSceneSubSequenceTree&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSubSequenceTree> : TStructOpsTypeTraitsBase2<FMovieSceneSubSequenceTree> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneSequenceHierarchy {
    GENERATED_BODY()

    UPROPERTY()
    FMovieSceneSequenceHierarchyNode RootNode;
    UPROPERTY()
    FMovieSceneSubSequenceTree Tree;
    UPROPERTY()
    TMap<FMovieSceneSequenceID, FMovieSceneSubSequenceData> SubSequences;
    UPROPERTY()
    TMap<FMovieSceneSequenceID, FMovieSceneSequenceHierarchyNode> Hierarchy;
    UPROPERTY()
    EMovieSceneServerClientMask AccumulatedNetworkMask = EMovieSceneServerClientMask::All;
    UPROPERTY()
    FMovieSceneSequenceTransform RootTransform;
};

}
