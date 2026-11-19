#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "IAudioExtensionPlugin.generated.h"

namespace ue5parse::runtime {

class USoundModulationPluginSourceSettingsBase;

USTRUCT()
struct FSoundModulation {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<USoundModulationPluginSourceSettingsBase>> Settings;
};

}
