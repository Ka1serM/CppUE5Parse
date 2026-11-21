#include "Engine/Source/Runtime/MovieScene/EntitySystem/MovieScenePropertyBinding.h"

namespace ue5parse::runtime {

FMovieScenePropertyBinding::FMovieScenePropertyBinding(FName InPropertyName, const FString& InPropertyPath)
    : PropertyName(InPropertyName), PropertyPath(MakeName(InPropertyPath)) {
    const std::string Path = InPropertyPath.ToUtf8();
    bCanUseClassLookup = Path.find_first_of("/\\[") == std::string::npos;
}

}
