#pragma once

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/LevelSequence/LevelSequenceBindingReference.h"
#include "Engine/Source/Runtime/LevelSequence/LevelSequenceLegacyObjectReference.h"
#include "Engine/Source/Runtime/LevelSequence/LevelSequenceObject.h"
#include "Engine/Source/Runtime/MovieScene/MovieScene.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSequence.h"

#include "LevelSequence.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULevelSequence : public UMovieSceneSequence {
    GENERATED_BODY()
public:
    explicit ULevelSequence(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FLevelSequenceObjectReferenceMap DefaultObjectReferences;
    UPROPERTY()
    TObjectPtr<UMovieScene> MovieScene;
    UPROPERTY()
    TMap<FString, FLevelSequenceObject> PossessedObjects_DEPRECATED;
    UPROPERTY()
    FLevelSequenceObjectReferenceMap ObjectReferences;
    UPROPERTY()
    FUpgradedLevelSequenceBindingReferences BindingReferences;
    UPROPERTY()
    TObjectPtr<UClass> DirectorClass;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
};

UCLASS()
class ULevelSequenceInstance : public UMovieSceneSequence {
    GENERATED_BODY()
public:
    explicit ULevelSequenceInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<ULevelSequence> LevelSequence;
    UPROPERTY()
    bool bCanRemapBindings = false;
    UPROPERTY()
    FLevelSequenceObjectReferenceMap RemappedObjectReferences;
};

}
