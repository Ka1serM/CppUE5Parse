#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "MetasoundSettings.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMetaSoundQualitySettings {
    GENERATED_BODY()

    UPROPERTY()
    FGuid UniqueId;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FPerPlatformInt SampleRate;
    UPROPERTY()
    FPerPlatformFloat BlockRate;
};

}
