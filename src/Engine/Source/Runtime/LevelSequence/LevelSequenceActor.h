#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LevelSequenceActor.generated.h"

namespace ue5parse::runtime {

class ULevelSequence;
class ULevelSequenceInstance;
class UMovieSceneBindingOverrides;

UCLASS(DefaultToInstanced)
class ULevelSequenceBurnInInitSettings : public UObject {
    GENERATED_BODY()
public:
    explicit ULevelSequenceBurnInInitSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(DefaultToInstanced)
class ULevelSequenceBurnInOptions : public UObject {
    GENERATED_BODY()
public:
    explicit ULevelSequenceBurnInOptions(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bUseBurnIn = false;
    UPROPERTY()
    FSoftClassPath BurnInClass;
    UPROPERTY()
    TObjectPtr<ULevelSequenceBurnInInitSettings> Settings = nullptr;
};

UCLASS()
class ALevelSequenceActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALevelSequenceActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bAutoPlay = false;
    UPROPERTY()
    TObjectPtr<ULevelSequence> LevelSequenceAsset = nullptr;
    UPROPERTY()
    FSoftObjectPath LevelSequence_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> AdditionalEventReceivers;
    UPROPERTY()
    TObjectPtr<ULevelSequenceBurnInOptions> BurnInOptions = nullptr;
    UPROPERTY()
    TObjectPtr<UMovieSceneBindingOverrides> BindingOverrides = nullptr;
    UPROPERTY()
    bool bOverrideInstanceData = false;
    UPROPERTY()
    bool bReplicatePlayback = false;
    UPROPERTY()
    TObjectPtr<UObject> DefaultInstanceData = nullptr;
    UPROPERTY()
    TObjectPtr<ULevelSequenceInstance> SequenceInstance = nullptr;
};

UCLASS()
class AReplicatedLevelSequenceActor : public ALevelSequenceActor {
    GENERATED_BODY()
public:
    explicit AReplicatedLevelSequenceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
