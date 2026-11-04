#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AkAudioEvent.generated.h"

namespace ue5parse::runtime {

enum class EWwiseLanguageRequirement : uint8 { IsDefault, IsOptional, SFX };

struct FWwiseLanguageCookedData {
    int32 LanguageId = 0;
    FName LanguageName;
    EWwiseLanguageRequirement LanguageRequirement = EWwiseLanguageRequirement::IsDefault;
};

struct FWwiseMediaCookedData {
    int32 MediaId = 0;
    FName MediaPathName;
    int32 PrefetchSize = 0;
    int32 MemoryAlignment = 0;
    bool bDeviceMemory = false;
    bool bStreaming = false;
    FName DebugName;
};

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
    TArray<TPair<FWwiseLanguageCookedData, FWwiseEventCookedData>> EventLanguageMap;
    FName DebugName;
    int32 EventId = 0;
};

UCLASS()
class UAkAudioType : public UObject {
    GENERATED_BODY()
public:
};

UCLASS()
class UAkAudioEvent : public UAkAudioType {
    GENERATED_BODY()
public:
    FWwiseLocalizedEventCookedData EventCookedData;

    void Serialize(FArchive& Ar) override;
};

}
