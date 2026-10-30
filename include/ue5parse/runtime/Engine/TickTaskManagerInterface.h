#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineBaseTypes.h"

namespace ue5parse::runtime {

class UWorld;

// FTickTaskManager: runs the registered tick functions of a world's levels by tick group, each after its
// prerequisites (UE's TickTaskManager.cpp). The runtime ticks on one thread.
class FTickTaskManager {
public:
    // The start of a frame: the enabled tick functions, each in the latest group of itself and its prerequisites.
    void StartFrame(UWorld* InWorld, float DeltaSeconds, ELevelTick TickType);
    // Runs a group's tick functions, then the ones registered meanwhile (UE's TG_NewlySpawned) until none is left.
    void RunTickGroup(ETickingGroup Group);
    void EndFrame();

    void AddTickFunction(FTickFunction* TickFunction);
    void RemoveTickFunction(FTickFunction* TickFunction);

private:
    void ScheduleTickFunction(FTickFunction* TickFunction);
    ETickingGroup ResolveTickGroup(FTickFunction* TickFunction, int32 Depth);
    void RunTickFunction(FTickFunction* TickFunction, int32 Depth);
    bool CanRun(const FTickFunction* TickFunction) const;

    TArray<FTickFunction*> AllTickFunctions;
    // The functions due this frame by actual start group, in registration order.
    TArray<FTickFunction*> Scheduled[TG_MAX];
    TArray<FTickFunction*> NewlySpawned;
    UWorld* World = nullptr;
    float DeltaSeconds = 0;
    ELevelTick TickType = LEVELTICK_All;
    int32 FrameCounter = 0;
    bool bTickingFrame = false;
    ETickingGroup CurrentGroup = TG_PrePhysics;
};

}
