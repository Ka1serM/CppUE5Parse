#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"

namespace ue5parse::runtime {

class UWorld;

class FTickTaskManager {
public:
    void StartFrame(UWorld* InWorld, float DeltaSeconds, ELevelTick TickType);
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
    void CompleteTickFunction(FTickFunction* TickFunction);
    void CompleteTickGroup(ETickingGroup Group);

    TArray<FTickFunction*> AllTickFunctions;
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
