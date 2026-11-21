#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTrack.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

bool FMovieSceneEvaluationTrackSegments::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("ArrayProperty"))
        return false;
    const FProperty* Property = StaticStruct()->FindPropertyByName(FName("SortedSegments"));
    Property->SerializeItem(Ar, &SortedSegments);
    SegmentIdentifierToIndex.SetNum(SortedSegments.Num());
    for (int32 Index = 0; Index < SortedSegments.Num(); ++Index) {
        SegmentIdentifierToIndex[Index] = Index;
        SortedSegments[Index].ID.IdentifierIndex = Index;
    }
    return true;
}

}
