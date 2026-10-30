#include "ue5parse/runtime/Engine/TimerManager.h"

#include <algorithm>

// UE's TimerManager.cpp (UE 5.3): a timer expires at InternalTime + delay; each tick runs the expired timers in the
// order they expire, a looping one as often as its rate fits into the elapsed time.

namespace ue5parse::runtime {

FTimerManager::FTimerData* FTimerManager::FindTimer(FTimerHandle InHandle) {
    if (!InHandle.IsValid()) return nullptr;
    for (FTimerData& Timer : Timers)
        if (Timer.Handle == InHandle) return &Timer;
    return nullptr;
}

const FTimerManager::FTimerData* FTimerManager::FindTimer(FTimerHandle InHandle) const {
    return const_cast<FTimerManager*>(this)->FindTimer(InHandle);
}

void FTimerManager::InternalSetTimer(FTimerHandle& InOutHandle, FTimerData&& Data, float InRate, float InFirstDelay) {
    ClearTimer(InOutHandle);
    if (InRate <= 0.0f) return;
    Data.Handle.Handle = ++LastAssignedSerialNumber;
    Data.Rate = InRate;
    const float FirstDelay = InFirstDelay >= 0.0f ? InFirstDelay : InRate;
    Data.ExpireTime = InternalTime + FirstDelay;
    Data.Status = ETimerStatus::Active;
    InOutHandle = Data.Handle;
    Timers.push_back(std::move(Data));
}

void FTimerManager::SetTimer(FTimerHandle& InOutHandle, const FScriptDelegate& InDelegate, float InRate, bool InbLoop, float InFirstDelay) {
    FTimerData Data;
    Data.DynamicDelegate = InDelegate;
    Data.bLoop = InbLoop;
    InternalSetTimer(InOutHandle, std::move(Data), InRate, InFirstDelay);
}

void FTimerManager::SetTimer(FTimerHandle& InOutHandle, FTimerFunction InFunction, float InRate, bool InbLoop, float InFirstDelay) {
    FTimerData Data;
    Data.Function = std::move(InFunction);
    Data.bLoop = InbLoop;
    InternalSetTimer(InOutHandle, std::move(Data), InRate, InFirstDelay);
}

FTimerHandle FTimerManager::K2_FindDynamicTimerHandle(const FScriptDelegate& InDynamicDelegate) const {
    for (const FTimerData& Timer : Timers)
        if (!Timer.Function && Timer.DynamicDelegate == InDynamicDelegate) return Timer.Handle;
    return FTimerHandle();
}

void FTimerManager::SetTimerForNextTick(FTimerFunction InFunction) { NextTickFunctions.push_back(std::move(InFunction)); }

// Expires now: the next tick, which advances the time first, runs it.
FTimerHandle FTimerManager::SetTimerForNextTick(const FScriptDelegate& InDynDelegate) {
    FTimerData Data;
    Data.DynamicDelegate = InDynDelegate;
    Data.Handle.Handle = ++LastAssignedSerialNumber;
    Data.ExpireTime = InternalTime;
    Timers.push_back(std::move(Data));
    return Timers.back().Handle;
}

void FTimerManager::ClearTimer(FTimerHandle& InHandle) {
    if (FTimerData* Timer = FindTimer(InHandle)) {
        // A timer that runs now is removed after it ran.
        if (Timer->Status == ETimerStatus::Executing) Timer->bLoop = false;
        else Timers.erase(Timers.begin() + (Timer - Timers.data()));
    }
    InHandle.Invalidate();
}

void FTimerManager::ClearAllTimersForObject(const UObject* Object) {
    if (Object == nullptr) return;
    std::erase_if(Timers, [&](const FTimerData& Timer) {
        return Timer.Status != ETimerStatus::Executing && !Timer.Function && Timer.DynamicDelegate.GetUObjectEvenIfUnreachable() == Object;
    });
}

void FTimerManager::PauseTimer(FTimerHandle InHandle) {
    FTimerData* Timer = FindTimer(InHandle);
    if (Timer == nullptr || Timer->Status == ETimerStatus::Paused) return;
    Timer->PausedRemaining = Timer->ExpireTime - InternalTime;
    Timer->Status = ETimerStatus::Paused;
}

void FTimerManager::UnPauseTimer(FTimerHandle InHandle) {
    FTimerData* Timer = FindTimer(InHandle);
    if (Timer == nullptr || Timer->Status != ETimerStatus::Paused) return;
    Timer->ExpireTime = InternalTime + Timer->PausedRemaining;
    Timer->Status = ETimerStatus::Active;
}

bool FTimerManager::IsTimerActive(FTimerHandle InHandle) const {
    const FTimerData* Timer = FindTimer(InHandle);
    return Timer && Timer->Status != ETimerStatus::Paused;
}

bool FTimerManager::IsTimerPaused(FTimerHandle InHandle) const {
    const FTimerData* Timer = FindTimer(InHandle);
    return Timer && Timer->Status == ETimerStatus::Paused;
}

float FTimerManager::GetTimerRate(FTimerHandle InHandle) const {
    const FTimerData* Timer = FindTimer(InHandle);
    return Timer ? Timer->Rate : -1.0f;
}

float FTimerManager::GetTimerElapsed(FTimerHandle InHandle) const {
    const FTimerData* Timer = FindTimer(InHandle);
    if (Timer == nullptr) return -1.0f;
    const double Remaining = Timer->Status == ETimerStatus::Paused ? Timer->PausedRemaining : Timer->ExpireTime - InternalTime;
    return static_cast<float>(Timer->Rate - Remaining);
}

float FTimerManager::GetTimerRemaining(FTimerHandle InHandle) const {
    const FTimerData* Timer = FindTimer(InHandle);
    if (Timer == nullptr) return -1.0f;
    return static_cast<float>(Timer->Status == ETimerStatus::Paused ? Timer->PausedRemaining : Timer->ExpireTime - InternalTime);
}

void FTimerManager::Execute(FTimerData& Timer) {
    if (Timer.Function) Timer.Function();
    else if (Timer.DynamicDelegate.IsBound()) Timer.DynamicDelegate.ProcessDelegate(nullptr);
}

void FTimerManager::Tick(float DeltaTime) {
    InternalTime += DeltaTime;
    std::vector<FTimerFunction> NextTick = std::move(NextTickFunctions);
    NextTickFunctions.clear();
    for (FTimerFunction& Function : NextTick) Function();
    while (true) {
        // The earliest expired timer; timers set during a run join with their own expire times.
        auto Next = Timers.end();
        for (auto It = Timers.begin(); It != Timers.end(); ++It)
            if (It->Status == ETimerStatus::Active && It->ExpireTime <= InternalTime && (Next == Timers.end() || It->ExpireTime < Next->ExpireTime))
                Next = It;
        if (Next == Timers.end()) return;
        const FTimerHandle Handle = Next->Handle;
        const int32 CallCount = Next->bLoop ? static_cast<int32>((InternalTime - Next->ExpireTime) / Next->Rate) + 1 : 1;
        Next->Status = ETimerStatus::Executing;
        for (int32 Call = 0; Call < CallCount; Call++) {
            FTimerData* Timer = FindTimer(Handle);
            if (Timer == nullptr) break;
            // A copy: the delegate may set timers, which moves the array.
            FTimerData Running = *Timer;
            Execute(Running);
            Timer = FindTimer(Handle);
            if (Timer == nullptr || Timer->Status != ETimerStatus::Executing) break;
        }
        FTimerData* Timer = FindTimer(Handle);
        if (Timer == nullptr || Timer->Status != ETimerStatus::Executing) continue;
        if (Timer->bLoop) {
            Timer->ExpireTime += CallCount * Timer->Rate;
            Timer->Status = ETimerStatus::Active;
        } else {
            Timers.erase(Timers.begin() + (Timer - Timers.data()));
        }
    }
}

}
