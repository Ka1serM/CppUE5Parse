#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"

#include "TimerHandle.generated.h"

namespace ue5parse::runtime {

using FTimerDynamicDelegate = FScriptDelegate;

USTRUCT()
struct FTimerHandle {
    GENERATED_BODY()
    UPROPERTY(Transient)
    uint64 Handle = 0;

    bool IsValid() const { return Handle != 0; }
    void Invalidate() { Handle = 0; }
    bool operator==(const FTimerHandle& Other) const { return Handle == Other.Handle; }
};

}
