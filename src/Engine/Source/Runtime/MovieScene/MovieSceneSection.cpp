#include "Engine/Source/Runtime/MovieScene/MovieSceneSection.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/MovieScene/Generators/MovieSceneEasingCurves.h"

namespace ue5parse::runtime {

UMovieSceneSection::UMovieSceneSection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SectionRange.LowerBound = {ERangeBoundTypes::Inclusive, FFrameNumber{0}};
    SectionRange.UpperBound = {ERangeBoundTypes::Inclusive, FFrameNumber{0}};
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_17)
        return;
    auto* EaseIn = ObjectInitializer.CreateDefaultSubobject<UMovieSceneBuiltInEasingFunction>(FName("EaseInFunction"));
    EaseIn->SetFlags(RF_Public);
    Easing.EaseIn.SetObject(EaseIn);
    auto* EaseOut = ObjectInitializer.CreateDefaultSubobject<UMovieSceneBuiltInEasingFunction>(FName("EaseOutFunction"));
    EaseOut->SetFlags(RF_Public);
    Easing.EaseOut.SetObject(EaseOut);
}

}
