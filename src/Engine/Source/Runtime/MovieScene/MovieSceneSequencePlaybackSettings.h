#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MovieSceneSequencePlaybackSettings.generated.h"

namespace ue5parse::runtime {

struct FPropertyTag;

UENUM()
enum class EMovieSceneCompletionModeOverride : uint8 { None, ForceKeepState, ForceRestoreState };

USTRUCT()
struct FMovieSceneSequenceLoopCount {
    GENERATED_BODY()

    UPROPERTY()
    int32 Value = 0;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSequenceLoopCount> : TStructOpsTypeTraitsBase2<FMovieSceneSequenceLoopCount> {
    enum { WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FMovieSceneSequenceTickInterval {
    GENERATED_BODY()

    UPROPERTY()
    float TickIntervalSeconds = 0.0f;
    UPROPERTY()
    float EvaluationBudgetMicroseconds = 0.0f;
    UPROPERTY()
    bool bTickWhenPaused = false;
    UPROPERTY()
    bool bAllowRounding = true;
};

USTRUCT()
struct FMovieSceneSequencePlaybackSettings {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bAutoPlay : 1 = false;
    UPROPERTY()
    FMovieSceneSequenceLoopCount LoopCount;
    UPROPERTY()
    FMovieSceneSequenceTickInterval TickInterval;
    UPROPERTY()
    float PlayRate = 1.0f;
    UPROPERTY()
    float StartTime = 0.0f;
    UPROPERTY()
    uint32 bRandomStartTime : 1 = false;
    UPROPERTY()
    uint32 bRestoreState_DEPRECATED : 1 = false;
    UPROPERTY()
    uint32 bDisableMovementInput : 1 = false;
    UPROPERTY()
    uint32 bDisableLookAtInput : 1 = false;
    UPROPERTY()
    uint32 bHidePlayer : 1 = false;
    UPROPERTY()
    uint32 bHideHud : 1 = false;
    UPROPERTY()
    uint32 bDisableCameraCuts : 1 = false;
    UPROPERTY()
    EMovieSceneCompletionModeOverride FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::None;
    UPROPERTY()
    uint32 bPauseAtEnd : 1 = false;
    UPROPERTY()
    uint32 bInheritTickIntervalFromOwner : 1 = true;
    UPROPERTY()
    uint32 bDynamicWeighting : 1 = false;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSequencePlaybackSettings> : TStructOpsTypeTraitsBase2<FMovieSceneSequencePlaybackSettings> {
    enum { WithSerializeFromMismatchedTag = true };
};

}
