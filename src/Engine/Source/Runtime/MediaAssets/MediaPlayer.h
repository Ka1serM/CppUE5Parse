#pragma once

#include "Engine/Source/Runtime/Core/Misc/Timespan.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "MediaPlayer.generated.h"

namespace ue5parse::runtime {

class UMediaTexture;

UENUM()
enum EMediaPlayerStreamModes : int32 { MASM_FromMemory, MASM_FromUrl, MASM_MAX };

UCLASS()
class UMediaPlayer : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString URL;
    UPROPERTY()
    TEnumAsByte<EMediaPlayerStreamModes> StreamMode = MASM_FromUrl;
    UPROPERTY()
    uint32 AutoPlay : 1 = false;
    UPROPERTY()
    float AutoPlayRate = 1.0f;
    UPROPERTY()
    uint32 Looping : 1 = false;
    UPROPERTY()
    TObjectPtr<UMediaTexture> ImageTexture = nullptr;
    UPROPERTY()
    TObjectPtr<UMediaTexture> VideoTexture = nullptr;
    UPROPERTY()
    FTimespan CacheAhead = FTimespan::FromMilliseconds(100);
    UPROPERTY()
    FTimespan CacheBehind = FTimespan::FromMilliseconds(3000);
    UPROPERTY()
    FTimespan CacheBehindGame = FTimespan::FromMilliseconds(100);
    UPROPERTY()
    bool NativeAudioOut = false;
    UPROPERTY()
    bool PlayOnOpen = true;
    UPROPERTY()
    uint32 Shuffle : 1 = false;
    UPROPERTY()
    uint32 Loop : 1 = false;
    UPROPERTY()
    int32 PlaylistIndex = INDEX_NONE;
    UPROPERTY()
    FTimespan TimeDelay;
    UPROPERTY()
    float HorizontalFieldOfView = 90.0f;
    UPROPERTY()
    float VerticalFieldOfView = 60.0f;
    UPROPERTY()
    FRotator ViewRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FGuid PlayerGuid;
};

}
