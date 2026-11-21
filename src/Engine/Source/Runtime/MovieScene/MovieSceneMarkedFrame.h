#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"

#include "MovieSceneMarkedFrame.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneMarkedFrame {
    GENERATED_BODY()

    UPROPERTY()
    FFrameNumber FrameNumber;
    UPROPERTY()
    FString Label;
    UPROPERTY()
    bool bIsDeterminismFence = false;
    UPROPERTY()
    bool bIsInclusiveTime = false;
};

}
