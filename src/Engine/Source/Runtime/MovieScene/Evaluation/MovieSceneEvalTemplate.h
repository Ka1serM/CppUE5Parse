#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFwd.h"

#include "MovieSceneEvalTemplate.generated.h"

namespace ue5parse::runtime {

class UMovieSceneSection;

bool SerializeMovieSceneInlineValue(FArchive& Ar, FString& TypeName, FInstancedStruct& Value);

USTRUCT()
struct FMovieSceneEmptyStruct {
    GENERATED_BODY()
};

USTRUCT()
struct FMovieSceneEvalTemplateBase {
    GENERATED_BODY()
};

USTRUCT()
struct FMovieSceneEvalTemplate : public FMovieSceneEvalTemplateBase {
    GENERATED_BODY()

    UPROPERTY()
    EMovieSceneCompletionMode CompletionMode = EMovieSceneCompletionMode::KeepState;
    UPROPERTY()
    TObjectPtr<UMovieSceneSection> SourceSection = nullptr;
    UPROPERTY()
    TWeakObjectPtr<UMovieSceneSection> SourceSectionPtr;
};

USTRUCT()
struct FMovieSceneTrackImplementation : public FMovieSceneEvalTemplateBase {
    GENERATED_BODY()
};

USTRUCT()
struct FMovieSceneEvalTemplatePtr {
    GENERATED_BODY()

    FString TypeName;
    FInstancedStruct Value;

    bool Serialize(FArchive& Ar) { return SerializeMovieSceneInlineValue(Ar, TypeName, Value); }
    bool operator==(const FMovieSceneEvalTemplatePtr&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEvalTemplatePtr> : TStructOpsTypeTraitsBase2<FMovieSceneEvalTemplatePtr> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneTrackImplementationPtr {
    GENERATED_BODY()

    FString TypeName;
    FInstancedStruct Value;

    bool Serialize(FArchive& Ar) { return SerializeMovieSceneInlineValue(Ar, TypeName, Value); }
    bool operator==(const FMovieSceneTrackImplementationPtr&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTrackImplementationPtr> : TStructOpsTypeTraitsBase2<FMovieSceneTrackImplementationPtr> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FMovieSceneSequenceInstanceData {
    GENERATED_BODY()
};

USTRUCT()
struct FMovieSceneSequenceInstanceDataPtr {
    GENERATED_BODY()

    FString TypeName;
    FInstancedStruct Value;

    bool Serialize(FArchive& Ar) { return SerializeMovieSceneInlineValue(Ar, TypeName, Value); }
    bool operator==(const FMovieSceneSequenceInstanceDataPtr&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSequenceInstanceDataPtr> : TStructOpsTypeTraitsBase2<FMovieSceneSequenceInstanceDataPtr> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
