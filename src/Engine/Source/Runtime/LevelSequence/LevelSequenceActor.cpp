#include "Engine/Source/Runtime/LevelSequence/LevelSequenceActor.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/LevelSequence/DefaultLevelSequenceInstanceData.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneBindingOverrides.h"

namespace ue5parse::runtime {

ALevelSequenceActor::ALevelSequenceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    const EGame Game = GetRuntimeProvider().getGame();
    if (Game >= EGame::GAME_UE4_14)
        RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    if (Game >= EGame::GAME_UE4_15)
        BindingOverrides = ObjectInitializer.CreateDefaultSubobject<UMovieSceneBindingOverrides>(FName("BindingOverrides"));
    if (Game >= EGame::GAME_UE4_13)
        BurnInOptions = ObjectInitializer.CreateDefaultSubobject<ULevelSequenceBurnInOptions>(FName("BurnInOptions"));
    if (Game >= EGame::GAME_UE4_19)
        DefaultInstanceData = ObjectInitializer.CreateDefaultSubobject<UDefaultLevelSequenceInstanceData>(FName("InstanceData"));
    bReplicates = true;
}

}
