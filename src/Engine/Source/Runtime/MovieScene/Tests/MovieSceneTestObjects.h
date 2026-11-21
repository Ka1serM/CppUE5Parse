#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MovieSceneTestObjects.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FTestMovieSceneStruct {
    GENERATED_BODY()

    UPROPERTY()
    float First = 0.0f;
    UPROPERTY()
    float Second = 0.0f;
    UPROPERTY()
    FVector Vector = FVector::ZeroVector;
};

UCLASS()
class ATestMovieSceneArrayPropertiesActor : public AActor {
    GENERATED_BODY()
public:
    explicit ATestMovieSceneArrayPropertiesActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FVector TestVector = FVector::ZeroVector;
    UPROPERTY()
    TArray<float> MultipleFloats;
    UPROPERTY()
    FTestMovieSceneStruct SingleStruct;
    UPROPERTY()
    TArray<FTestMovieSceneStruct> MultipleStructs;
};

}
