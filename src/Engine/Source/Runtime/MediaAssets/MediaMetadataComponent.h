#pragma once

#include "Engine/Source/Runtime/Core/Misc/Timespan.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MediaMetadataComponent.generated.h"

namespace ue5parse::runtime {

class UMediaMetadataComponent;
class UMediaPlayer;

USTRUCT()
struct FMediaMetadataSample {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint8> Data;
    UPROPERTY()
    FTimespan Time;
    UPROPERTY()
    FTimecode Timecode;
    UPROPERTY()
    bool bHasTimecode = false;
    UPROPERTY()
    FGuid TypeId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMediaMetadataSampleReceived, UMediaMetadataComponent*, Component, const FMediaMetadataSample&, Sample);

UCLASS()
class UMediaMetadataComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMediaMetadataComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FOnMediaMetadataSampleReceived OnSampleReceived;
    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer = nullptr;
};

}
