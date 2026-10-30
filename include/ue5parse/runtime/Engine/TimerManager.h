#pragma once

#include <functional>
#include <vector>

#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/Engine/Engine/TimerHandle.h"

namespace ue5parse::runtime {

// FTimerManager: the timers of a world, run in its tick after the tick groups up to TG_PostPhysics (UE's
// Engine/Public/TimerManager.h). A timer calls a Blueprint delegate or a C++ function.
class FTimerManager {
public:
    using FTimerFunction = std::function<void()>;

    // Replaces the handle's timer. A rate of 0 or less clears it. FirstDelay < 0 uses the rate.
    void SetTimer(FTimerHandle& InOutHandle, const FScriptDelegate& InDelegate, float InRate, bool InbLoop, float InFirstDelay = -1.0f);
    void SetTimer(FTimerHandle& InOutHandle, FTimerFunction InFunction, float InRate, bool InbLoop, float InFirstDelay = -1.0f);
    // The timer of the delegate (K2_SetTimerDelegate replaces it).
    FTimerHandle K2_FindDynamicTimerHandle(const FScriptDelegate& InDynamicDelegate) const;
    void SetTimerForNextTick(FTimerFunction InFunction);
    // A timer that calls the delegate once on the next tick (UE's SetTimerForNextTick of a dynamic delegate).
    FTimerHandle SetTimerForNextTick(const FScriptDelegate& InDynDelegate);
    void ClearTimer(FTimerHandle& InHandle);
    void ClearAllTimersForObject(const UObject* Object);
    void PauseTimer(FTimerHandle InHandle);
    void UnPauseTimer(FTimerHandle InHandle);
    bool IsTimerActive(FTimerHandle InHandle) const;
    bool IsTimerPaused(FTimerHandle InHandle) const;
    bool TimerExists(FTimerHandle InHandle) const { return FindTimer(InHandle) != nullptr; }
    float GetTimerRate(FTimerHandle InHandle) const;
    float GetTimerElapsed(FTimerHandle InHandle) const;
    float GetTimerRemaining(FTimerHandle InHandle) const;
    // Runs the timers that expire in this frame, in the order they expire.
    void Tick(float DeltaTime);

private:
    enum class ETimerStatus : uint8 { Pending, Active, Paused, Executing };
    struct FTimerData {
        FTimerHandle Handle;
        FScriptDelegate DynamicDelegate;
        FTimerFunction Function;
        double ExpireTime = 0;
        float Rate = 0;
        bool bLoop = false;
        ETimerStatus Status = ETimerStatus::Active;
        // A paused timer keeps its time left here.
        double PausedRemaining = 0;
    };
    FTimerData* FindTimer(FTimerHandle InHandle);
    const FTimerData* FindTimer(FTimerHandle InHandle) const;
    void InternalSetTimer(FTimerHandle& InOutHandle, FTimerData&& Data, float InRate, float InFirstDelay);
    void Execute(FTimerData& Timer);

    std::vector<FTimerData> Timers;
    double InternalTime = 0;
    uint64 LastAssignedSerialNumber = 0;
    std::vector<FTimerFunction> NextTickFunctions;
};

}
