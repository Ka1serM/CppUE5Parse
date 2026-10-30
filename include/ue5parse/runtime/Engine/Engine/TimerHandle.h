#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"

#include "TimerHandle.generated.h"

namespace ue5parse::runtime {

// The Blueprint delegate a timer calls (UE's DECLARE_DYNAMIC_DELEGATE(FTimerDynamicDelegate)).
using FTimerDynamicDelegate = FScriptDelegate;

// FTimerHandle: the id of a timer of a FTimerManager (UE's Engine/TimerHandle.h). 0 is no timer.
USTRUCT()
struct FTimerHandle {
    GENERATED_BODY()
    UPROPERTY()
    uint64 Handle = 0;

    bool IsValid() const { return Handle != 0; }
    void Invalidate() { Handle = 0; }
    bool operator==(const FTimerHandle& Other) const { return Handle == Other.Handle; }
};

}
