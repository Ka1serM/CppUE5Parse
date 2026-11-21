#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/MovieScene/Bindings/MovieSceneCustomBinding.h"
#include "Engine/Source/Runtime/UniversalObjectLocator/UniversalObjectLocator.h"
#include "Engine/Source/Runtime/UniversalObjectLocator/UniversalObjectLocatorResolveParams.h"

#include "MovieSceneBindingReferences.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneBindingReference {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ID;
    UPROPERTY()
    FUniversalObjectLocator Locator;
    UPROPERTY()
    ELocatorResolveFlags ResolveFlags = ELocatorResolveFlags::None;
    UPROPERTY()
    TObjectPtr<UMovieSceneCustomBinding> CustomBinding;
};

USTRUCT()
struct FMovieSceneBindingReferences {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneBindingReference> SortedReferences;
};

}
