#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneDynamicBinding.h"

#include "MovieSceneSpawnable.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESpawnOwnership : uint8 { InnerSequence, RootSequence, External };

USTRUCT()
struct FMovieSceneSpawnable {
    GENERATED_BODY()

    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    FString Name;
    UPROPERTY()
    TObjectPtr<UClass> GeneratedClass = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> ObjectTemplate = nullptr;
    UPROPERTY()
    TArray<FGuid> ChildPossessables;
    UPROPERTY()
    ESpawnOwnership Ownership = ESpawnOwnership::InnerSequence;
    UPROPERTY()
    FTransform SpawnTransform;
    UPROPERTY()
    TArray<FName> Tags;
    UPROPERTY()
    FName LevelName;
    UPROPERTY()
    bool bContinuouslyRespawn = false;
    UPROPERTY()
    bool bEvaluateTracksWhenNotSpawned = false;
    UPROPERTY()
    bool bNetAddressableName = false;
    UPROPERTY()
    FMovieSceneDynamicBinding DynamicBinding;
};

}
