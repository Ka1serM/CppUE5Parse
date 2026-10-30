#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AnimNotifyState.generated.h"

namespace ue5parse::runtime {

// UAnimNotifyState (UE's Animation/AnimNotifies/AnimNotifyState.h): an event over a time range of an animation.
UCLASS(Abstract)
class UAnimNotifyState : public UObject {
    GENERATED_BODY()
};

}
