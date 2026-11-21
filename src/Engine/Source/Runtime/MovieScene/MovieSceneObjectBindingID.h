#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MovieSceneObjectBindingID.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMovieSceneObjectBindingSpace : uint8 { Local, Root, Unused };

USTRUCT()
struct FMovieSceneObjectBindingID {
    GENERATED_BODY()

    UPROPERTY()
    int32 SequenceID = 0;
    UPROPERTY()
    EMovieSceneObjectBindingSpace Space = EMovieSceneObjectBindingSpace::Root;
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    int32 ResolveParentIndex = 0;
};

}
