#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MovieSceneSignedObject.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMovieSceneSignedObject : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieSceneSignedObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FGuid Signature;
};

}
