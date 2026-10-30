#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineBaseTypes.h"

namespace ue5parse::runtime {

class UWorld;

// FTickTaskManager: runs the registered tick functions of a world's levels by tick group, each after its
// prerequisites (UE's TickTaskManager.cpp). The tick functions run on the calling thread; a tick can leave work on worker
// threads, whose completion task the manager runs on the calling thread (FTickFunction::DontCompleteUntil).
class FTickTaskManager {
public:
    // The start of a frame: the enabled tick functions, each in the latest group of itself and its prerequisites.
    void StartFrame(UWorld* InWorld, float DeltaSeconds, ELevelTick TickType);
    // Runs a group's tick functions, then the ones registered meanwhile (UE's TG_NewlySpawned) until none is left.
    void RunTickGroup(ETickingGroup Group);
    void EndFrame();

    void AddTickFunction(FTickFunction* TickFunction);
    void RemoveTickFunction(FTickFunction* TickFunction);
    void AddCompletion(FTickFunction* TickFunction);

private:
    void ScheduleTickFunction(FTickFunction* TickFunction);
    ETickingGroup ResolveTickGroup(FTickFunction* TickFunction, int32 Depth);
    void RunTickFunction(FTickFunction* TickFunction, int32 Depth);
    bool CanRun(const FTickFunction* TickFunction) const;
    // Runs the completion task of a tick function that has one (UE's wait on its completion handle).
    void CompleteTickFunction(FTickFunction* TickFunction);
    // The completion tasks of the tick functions whose end group is at most the group, in the order they were added.
    void CompleteTickGroup(ETickingGroup Group);

    TArray<FTickFunction*> AllTickFunctions;
    // The functions due this frame by actual start group, in registration order.
    TArray<FTickFunction*> Scheduled[TG_MAX];
    TArray<FTickFunction*> NewlySpawned;
    TArray<FTickFunction*> PendingCompletions;
    UWorld* World = nullptr;
    float DeltaSeconds = 0;
    ELevelTick TickType = LEVELTICK_All;
    int32 FrameCounter = 0;
    bool bTickingFrame = false;
    ETickingGroup CurrentGroup = TG_PrePhysics;
};

}
