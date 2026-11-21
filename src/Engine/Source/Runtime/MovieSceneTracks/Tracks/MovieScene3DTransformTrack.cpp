#include "Engine/Source/Runtime/MovieSceneTracks/Tracks/MovieScene3DTransformTrack.h"

namespace ue5parse::runtime {

UMovieScene3DTransformTrack::UMovieScene3DTransformTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SetPropertyNameAndPath(FName("Transform"), FString("Transform"));
}

}
