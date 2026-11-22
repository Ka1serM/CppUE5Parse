#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MovieSceneFwd.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMovieSceneEvaluationType : uint8 { FrameLocked, WithSubFrames };

UENUM()
enum class EUpdateClockSource : uint8 { Tick, Platform, Audio, RelativeTimecode, Timecode, PlayEveryFrame, Custom };

UENUM()
enum class EMovieSceneSequenceFlags : uint8 { None = 0, Volatile = 1 << 0, BlockingEvaluation = 1 << 1 };

UENUM()
enum class EMovieSceneCompletionMode : uint8 { KeepState, RestoreState, ProjectDefault };

}
