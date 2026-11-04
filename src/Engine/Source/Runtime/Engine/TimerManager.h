#pragma once

#include <functional>
#include <vector>

#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/Engine/Engine/TimerHandle.h"

namespace ue5parse::runtime {

class FTimerManager {
public:
    using FTimerFunction = std::function<void()>;

    void SetTimer(FTimerHandle& InOutHandle, const FScriptDelegate& InDelegate, float InRate, bool InbLoop, float InFirstDelay = -1.0f);
    void SetTimer(FTimerHandle& InOutHandle, FTimerFunction InFunction, float InRate, bool InbLoop, float InFirstDelay = -1.0f);
    FTimerHandle K2_FindDynamicTimerHandle(const FScriptDelegate& InDynamicDelegate) const;
    void SetTimerForNextTick(FTimerFunction InFunction);
    FTimerHandle SetTimerForNextTick(const FScriptDelegate& InDynDelegate);
    void ClearTimer(FTimerHandle& InHandle);
    void ClearAllTimersForObject(const UObject* Object);
    void PauseTimer(FTimerHandle InHandle);
    void UnPauseTimer(FTimerHandle InHandle);
    bool IsTimerActive(FTimerHandle InHandle) const;
    bool IsTimerPaused(FTimerHandle InHandle) const;
    bool TimerExists(FTimerHandle InHandle) const { return FindTimer(InHandle) != nullptr; }
    float GetTimerElapsed(FTimerHandle InHandle) const;
    float GetTimerRemaining(FTimerHandle InHandle) const;
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
