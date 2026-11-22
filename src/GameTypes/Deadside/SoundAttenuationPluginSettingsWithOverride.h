#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "SoundAttenuationPluginSettingsWithOverride.generated.h"

namespace ue5parse::runtime {

//! Deadside saves a bool and then, when it is set, the three arrays of plugin settings.
USTRUCT()
struct FSoundAttenuationPluginSettingsWithOverride {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<UObject>> SpatializationPluginSettingsArray;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> OcclusionPluginSettingsArray;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> ReverbPluginSettingsArray;

    bool Serialize(FArchive& Ar) {
        bool bOverride = false;
        Ar << bOverride;
        if (bOverride)
            Ar << SpatializationPluginSettingsArray << OcclusionPluginSettingsArray << ReverbPluginSettingsArray;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSoundAttenuationPluginSettingsWithOverride> : TStructOpsTypeTraitsBase2<FSoundAttenuationPluginSettingsWithOverride> {
    enum { WithSerializer = true };
};

}
