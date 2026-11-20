#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureDefines.h"

#include "MediaTexture.generated.h"

namespace ue5parse::runtime {

class UMediaPlayer;

UENUM()
enum MediaTextureOutputFormat : int32 { MTOF_Default, MTOF_SRGB_LINOUT, MTOF_MAX };

UCLASS()
class UMediaTexture : public UTexture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressX = TA_Clamp;
    UPROPERTY()
    TEnumAsByte<TextureAddress> AddressY = TA_Clamp;
    UPROPERTY()
    bool AutoClear = false;
    UPROPERTY()
    FLinearColor ClearColor = FLinearColor::Black;
    UPROPERTY()
    bool EnableGenMips = false;
    UPROPERTY()
    uint8 NumMips = 1;
    UPROPERTY()
    TEnumAsByte<MediaTextureOutputFormat> OutputFormat = MTOF_Default;
    UPROPERTY()
    bool NewStyleOutput = false;
    UPROPERTY()
    int32 VideoTrackIndex = 0;
    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer = nullptr;
    UPROPERTY()
    TWeakObjectPtr<UMediaPlayer> CurrentMediaPlayer;
};

}
