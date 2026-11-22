#pragma once

#include "Engine/Source/Runtime/Core/Containers/Set.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSignedObject.h"

#include "MovieSceneDecorationContainer.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneDecorationContainer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UObject>> Decorations;
};

UCLASS()
class UMovieSceneDecorationContainerObject : public UMovieSceneSignedObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneDecorationContainerObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FMovieSceneDecorationContainer Decorations;
    UPROPERTY(Transient)
    TSet<TObjectPtr<UObject>> MutedDecorations;
    UPROPERTY()
    TSet<TObjectPtr<UObject>> DisabledDecorations;
};

}
