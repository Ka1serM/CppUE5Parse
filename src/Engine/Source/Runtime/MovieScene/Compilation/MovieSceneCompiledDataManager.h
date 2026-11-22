#pragma once

#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTemplate.h"

#include "MovieSceneCompiledDataManager.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneDeterminismFence {
    GENERATED_BODY()

    UPROPERTY()
    FFrameNumber FrameNumber;
    UPROPERTY()
    uint8 bInclusive : 1 = false;

    //! Up to 5.4 the fences are frame times.
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneDeterminismFence> : TStructOpsTypeTraitsBase2<FMovieSceneDeterminismFence> {
    enum { WithSerializeFromMismatchedTag = true };
};

//! 5.6 replaced the struct with the one-byte EMovieSceneSequenceCompilerMask flags, whose bits are these fields.
USTRUCT()
struct FMovieSceneSequenceCompilerMaskStruct {
    GENERATED_BODY()

    UPROPERTY()
    uint8 bHierarchy : 1 = false;
    UPROPERTY()
    uint8 bEvaluationTemplate : 1 = false;
    UPROPERTY()
    uint8 bEvaluationTemplateField : 1 = false;
    UPROPERTY()
    uint8 bEntityComponentField : 1 = false;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSequenceCompilerMaskStruct> : TStructOpsTypeTraitsBase2<FMovieSceneSequenceCompilerMaskStruct> {
    enum { WithSerializeFromMismatchedTag = true };
};

UCLASS()
class UMovieSceneCompiledData : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneCompiledData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FMovieSceneEvaluationTemplate EvaluationTemplate;
    UPROPERTY()
    FMovieSceneSequenceHierarchy Hierarchy;
    UPROPERTY()
    FMovieSceneEntityComponentField EntityComponentField;
    UPROPERTY()
    FMovieSceneEvaluationField TrackTemplateField;
    UPROPERTY()
    TArray<FMovieSceneDeterminismFence> DeterminismFences;
    UPROPERTY()
    FGuid CompiledSignature;
    UPROPERTY()
    FGuid CompilerVersion;
    UPROPERTY()
    FMovieSceneSequenceCompilerMaskStruct AccumulatedMask;
    UPROPERTY()
    FMovieSceneSequenceCompilerMaskStruct AllocatedMask;
    UPROPERTY()
    EMovieSceneSequenceFlags AccumulatedFlags = EMovieSceneSequenceFlags::None;
};

}
