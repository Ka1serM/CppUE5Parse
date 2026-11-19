#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SkinWeightProfile.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSkinWeightProfileInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FPerPlatformBool DefaultProfile;
    UPROPERTY()
    FPerPlatformInt DefaultProfileFromLODIndex;
};

}
