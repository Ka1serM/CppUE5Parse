#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AnimNotify.generated.h"

namespace ue5parse::runtime {

// UAnimNotify (UE's Animation/AnimNotifies/AnimNotify.h): an event at one time of an animation.
UCLASS(Abstract)
class UAnimNotify : public UObject {
    GENERATED_BODY()
};

}
