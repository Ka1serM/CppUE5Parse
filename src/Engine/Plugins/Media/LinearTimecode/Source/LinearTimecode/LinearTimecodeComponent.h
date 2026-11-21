#pragma once

#include "Engine/Plugins/Media/LinearTimecode/Source/LinearTimecode/DropTimecode.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LinearTimecodeComponent.generated.h"

namespace ue5parse::runtime {

class UMediaPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChange, const FDropTimecode&, Timecode);

UCLASS()
class ULinearTimecodeComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULinearTimecodeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer = nullptr;
    UPROPERTY()
    FDropTimecode DropTimecode;
    UPROPERTY()
    FOnTimecodeChange OnTimecodeChange;
};

}
