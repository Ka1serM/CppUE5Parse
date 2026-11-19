#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ReverbSettings.generated.h"

namespace ue5parse::runtime {

class UReverbEffect;
class USoundEffectSubmixPreset;

UENUM()
enum ReverbPreset : int32 {
    REVERB_Default,
    REVERB_Bathroom,
    REVERB_StoneRoom,
    REVERB_Auditorium,
    REVERB_ConcertHall,
    REVERB_Cave,
    REVERB_Hallway,
    REVERB_StoneCorridor,
    REVERB_Alley,
    REVERB_Forest,
    REVERB_City,
    REVERB_Mountains,
    REVERB_Quarry,
    REVERB_Plain,
    REVERB_ParkingLot,
    REVERB_SewerPipe,
    REVERB_Underwater,
    REVERB_SmallRoom,
    REVERB_MediumRoom,
    REVERB_LargeRoom,
    REVERB_MediumHall,
    REVERB_LargeHall,
    REVERB_Plate,
    REVERB_MAX
};

USTRUCT()
struct FReverbSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bApplyReverb = true;
    UPROPERTY()
    TEnumAsByte<ReverbPreset> ReverbType_DEPRECATED = REVERB_Default;
    UPROPERTY()
    TObjectPtr<UReverbEffect> ReverbEffect = nullptr;
    UPROPERTY()
    TObjectPtr<USoundEffectSubmixPreset> ReverbPluginEffect = nullptr;
    UPROPERTY()
    float Volume = 0.5f;
    UPROPERTY()
    float FadeTime = 2.0f;
};

}
