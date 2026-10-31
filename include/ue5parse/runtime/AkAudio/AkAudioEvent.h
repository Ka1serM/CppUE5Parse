#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "AkAudioEvent.generated.h"

namespace ue5parse::runtime {

// The cooked data of the Wwise plugin's WwiseResourceLoader module, with the members that name the media. UE's types
// also hold TSet members, which have no C++ view here, so these are plain values that UAkAudioEvent fills.

enum class EWwiseLanguageRequirement : uint8 { IsDefault, IsOptional, SFX };

struct FWwiseLanguageCookedData {
    int32 LanguageId = 0;
    // "en-US". "SFX" for the media of every language.
    FName LanguageName;
    EWwiseLanguageRequirement LanguageRequirement = EWwiseLanguageRequirement::IsDefault;
};

// A WEM file of the Wwise project.
struct FWwiseMediaCookedData {
    int32 MediaId = 0;
    // Under the WwiseAudio folder of the content: "Media/160593138.wem", or "Media/en-US/…" for speech.
    FName MediaPathName;
    int32 PrefetchSize = 0;
    int32 MemoryAlignment = 0;
    bool bDeviceMemory = false;
    bool bStreaming = false;
    // The source file of the Wwise project: "Weapons\_Guns_Firing\Wp_AK47_Fire\Wp_AK47_Fire_Mech_5.wav".
    FName DebugName;
};

// The media that one switch value of a switch container plays.
struct FWwiseSwitchContainerLeafCookedData {
    TArray<FWwiseMediaCookedData> Media;
};

struct FWwiseEventCookedData {
    int32 EventId = 0;
    TArray<FWwiseMediaCookedData> Media;
    TArray<FWwiseSwitchContainerLeafCookedData> SwitchContainerLeaves;
    FName DebugName;
};

struct FWwiseLocalizedEventCookedData {
    // The event's data for each language, in the cooked order.
    TArray<TPair<FWwiseLanguageCookedData, FWwiseEventCookedData>> EventLanguageMap;
    FName DebugName;
    int32 EventId = 0;
};

UCLASS()
class UAkAudioType : public UObject {
    GENERATED_BODY()
public:
};

// UE's AkAudioEvent.h: a Wwise event. Its cooked data follows the properties.
UCLASS()
class UAkAudioEvent : public UAkAudioType {
    GENERATED_BODY()
public:
    FWwiseLocalizedEventCookedData EventCookedData;

    void Serialize(FArchive& Ar) override;
};

}
