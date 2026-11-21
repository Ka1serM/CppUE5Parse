#include "Engine/Source/Runtime/MovieScene/MovieSceneSequencePlaybackSettings.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FMovieSceneSequenceLoopCount::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("IntProperty"))
        return false;
    Ar << Value;
    return true;
}

bool FMovieSceneSequencePlaybackSettings::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("LevelSequencePlaybackSettings"))
        return false;
    StaticStruct()->SerializeItem(Ar, this);
    return true;
}

}
