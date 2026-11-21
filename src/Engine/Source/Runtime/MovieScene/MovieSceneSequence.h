#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTemplate.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFwd.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSignedObject.h"

#include "MovieSceneSequence.generated.h"

namespace ue5parse::runtime {

class UMovieSceneCompiledData;

UCLASS()
class UMovieSceneSequence : public UMovieSceneSignedObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneSequence(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FCachedMovieSceneEvaluationTemplate EvaluationTemplate;
    UPROPERTY()
    FMovieSceneTrackCompilationParams TemplateParameters;
    UPROPERTY()
    TMap<TObjectPtr<UObject>, FCachedMovieSceneEvaluationTemplate> InstancedSubSequenceEvaluationTemplates;
    UPROPERTY()
    FMovieSceneEvaluationTemplate PrecompiledEvaluationTemplate;
    UPROPERTY()
    TObjectPtr<UMovieSceneCompiledData> CompiledData = nullptr;
    UPROPERTY()
    EMovieSceneCompletionMode DefaultCompletionMode = EMovieSceneCompletionMode::RestoreState;
    UPROPERTY()
    bool bParentContextsAreSignificant = false;
    UPROPERTY()
    bool bPlayableDirectly = true;
    UPROPERTY()
    EMovieSceneSequenceFlags SequenceFlags = EMovieSceneSequenceFlags::None;
};

}
