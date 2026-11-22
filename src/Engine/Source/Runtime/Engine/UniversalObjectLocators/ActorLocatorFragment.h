#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ActorLocatorFragment.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FActorLocatorFragment {
    GENERATED_BODY()

    UPROPERTY()
    FSoftObjectPath Path;
};

}
