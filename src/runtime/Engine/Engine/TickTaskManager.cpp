#include "ue5parse/runtime/Engine/Engine/Level.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/TickTaskManagerInterface.h"

// UE's TickTaskManager.cpp (UE 5.3), on the calling thread: each enabled tick function runs once a frame in the latest
// tick group of itself and its prerequisites, after its prerequisites and their completion. Tick functions registered while a frame ticks run in
// that frame after the group that registered them (UE's TG_NewlySpawned).

namespace ue5parse::runtime {

namespace {

// A prerequisite cycle in data cannot recurse without end.
constexpr int32 MaxPrerequisiteDepth = 32;

FTickTaskManager* FindTickManager(ULevel* Level) {
    UWorld* World = Level ? Level->OwningWorld : nullptr;
    return World ? &World->GetTickTaskManager() : nullptr;
}

}

FTickFunction::~FTickFunction() { UnRegisterTickFunction(); }

void FTickFunction::RegisterTickFunction(ULevel* Level) {
    if (bRegistered) return;
    FTickTaskManager* Manager = FindTickManager(Level);
    if (Manager == nullptr) return;
    TickLevel = Level;
    Manager->AddTickFunction(this);
    bRegistered = true;
}

void FTickFunction::UnRegisterTickFunction() {
    if (!bRegistered) return;
    if (FTickTaskManager* Manager = FindTickManager(TickLevel)) Manager->RemoveTickFunction(this);
    bRegistered = false;
    TickLevel = nullptr;
}

void FTickFunction::DontCompleteUntil(std::function<void()> InCompletionTask) {
    FTickTaskManager* Manager = FindTickManager(TickLevel);
    UE5PARSE_CHECK(Manager != nullptr && !CompletionTask);
    CompletionTask = std::move(InCompletionTask);
    Manager->AddCompletion(this);
}

void FTickFunction::SetTickFunctionEnable(bool bInEnabled) {
    if (bInEnabled == (TickState != ETickState::Disabled)) return;
    TickState = bInEnabled ? ETickState::Enabled : ETickState::Disabled;
    if (bInEnabled) RelativeTickCooldown = 0;
}

void FTickFunction::UpdateTickIntervalAndCoolDown(float NewTickInterval) {
    TickInterval = NewTickInterval;
    if (TickState == ETickState::CoolingDown) RelativeTickCooldown = FMath::Min(RelativeTickCooldown, NewTickInterval);
}

void FTickFunction::AddPrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction) {
    const bool bThisCanTick = bCanEverTick || IsTickFunctionRegistered();
    const bool bTargetCanTick = TargetTickFunction.bCanEverTick || TargetTickFunction.IsTickFunctionRegistered();
    if (!bThisCanTick || !bTargetCanTick) return;
    FTickPrerequisite Prerequisite;
    Prerequisite.PrerequisiteObject = TargetObject;
    Prerequisite.PrerequisiteTickFunction = &TargetTickFunction;
    Prerequisites.AddUnique(Prerequisite);
}

void FTickFunction::RemovePrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction) {
    FTickPrerequisite Prerequisite;
    Prerequisite.PrerequisiteObject = TargetObject;
    Prerequisite.PrerequisiteTickFunction = &TargetTickFunction;
    Prerequisites.Remove(Prerequisite);
}

void FTickFunction::SetPriorityIncludingPrerequisites(bool bInHighPriority) { bHighPriority = bInHighPriority; }

void FTickTaskManager::AddTickFunction(FTickFunction* TickFunction) {
    AllTickFunctions.Add(TickFunction);
    if (bTickingFrame) NewlySpawned.Add(TickFunction);
}

void FTickTaskManager::RemoveTickFunction(FTickFunction* TickFunction) {
    AllTickFunctions.Remove(TickFunction);
    NewlySpawned.Remove(TickFunction);
    // The owner waits for its work as it unregisters (USkeletalMeshComponent::OnUnregister).
    PendingCompletions.Remove(TickFunction);
    TickFunction->CompletionTask = nullptr;
    for (TArray<FTickFunction*>& Group : Scheduled) Group.Remove(TickFunction);
}

void FTickTaskManager::AddCompletion(FTickFunction* TickFunction) { PendingCompletions.Add(TickFunction); }

void FTickTaskManager::CompleteTickFunction(FTickFunction* TickFunction) {
    if (!TickFunction->CompletionTask) return;
    const std::function<void()> Task = std::move(TickFunction->CompletionTask);
    TickFunction->CompletionTask = nullptr;
    PendingCompletions.Remove(TickFunction);
    Task();
}

void FTickTaskManager::CompleteTickGroup(ETickingGroup Group) {
    // A completion task can unregister tick functions, so the search starts again after each one.
    for (;;) {
        const int32 Next =
            PendingCompletions.IndexOfByPredicate([Group](const FTickFunction* TickFunction) { return TickFunction->ActualEndTickGroup <= Group; });
        if (Next == INDEX_NONE) return;
        CompleteTickFunction(PendingCompletions[Next]);
    }
}

ETickingGroup FTickTaskManager::ResolveTickGroup(FTickFunction* TickFunction, int32 Depth) {
    if (TickFunction->TickVisitedGFrameCounter == FrameCounter) return TickFunction->ActualStartTickGroup.GetValue();
    TickFunction->TickVisitedGFrameCounter = FrameCounter;
    ETickingGroup Group = TickFunction->TickGroup.GetValue();
    if (Depth < MaxPrerequisiteDepth)
        for (const FTickPrerequisite& Prerequisite : TickFunction->Prerequisites) {
            FTickFunction* Before = Prerequisite.Get();
            if (Before == nullptr || !Before->bRegistered || Before->TickState != FTickFunction::ETickState::Enabled) continue;
            Group = std::max(Group, ResolveTickGroup(Before, Depth + 1));
        }
    TickFunction->ActualStartTickGroup = Group;
    TickFunction->ActualEndTickGroup = std::max(Group, TickFunction->EndTickGroup.GetValue());
    return Group;
}

bool FTickTaskManager::CanRun(const FTickFunction* TickFunction) const {
    return TickFunction->bRegistered && TickFunction->TickState == FTickFunction::ETickState::Enabled;
}

void FTickTaskManager::ScheduleTickFunction(FTickFunction* TickFunction) {
    if (TickFunction->TickState == FTickFunction::ETickState::Disabled) return;
    // A tick interval: the function waits its cool down, then ticks with the time since its last tick.
    if (TickFunction->TickState == FTickFunction::ETickState::CoolingDown) {
        TickFunction->RelativeTickCooldown -= DeltaSeconds;
        if (TickFunction->RelativeTickCooldown > 0) return;
        TickFunction->TickState = FTickFunction::ETickState::Enabled;
    }
    ETickingGroup Group = ResolveTickGroup(TickFunction, 0);
    Scheduled[Group].Add(TickFunction);
}

void FTickTaskManager::StartFrame(UWorld* InWorld, float InDeltaSeconds, ELevelTick InTickType) {
    World = InWorld;
    DeltaSeconds = InDeltaSeconds;
    TickType = InTickType;
    FrameCounter++;
    bTickingFrame = true;
    NewlySpawned.Reset();
    for (TArray<FTickFunction*>& Group : Scheduled) Group.Reset();
    for (FTickFunction* TickFunction : AllTickFunctions) ScheduleTickFunction(TickFunction);
}

void FTickTaskManager::RunTickFunction(FTickFunction* TickFunction, int32 Depth) {
    if (TickFunction->TickVisitedGFrameCounter == -FrameCounter || !CanRun(TickFunction)) return;
    // Visited this frame as run: a negative frame counter.
    TickFunction->TickVisitedGFrameCounter = -FrameCounter;
    if (Depth < MaxPrerequisiteDepth)
        for (const FTickPrerequisite& Prerequisite : TickFunction->Prerequisites) {
            FTickFunction* Before = Prerequisite.Get();
            if (Before != nullptr && Before->ActualStartTickGroup.GetValue() <= CurrentGroup && Scheduled[CurrentGroup].Contains(Before))
                RunTickFunction(Before, Depth + 1);
        }
    for (const FTickPrerequisite& Prerequisite : TickFunction->Prerequisites)
        if (FTickFunction* Before = Prerequisite.Get()) CompleteTickFunction(Before);
    float TickDelta = DeltaSeconds;
    if (TickFunction->TickInterval > 0) {
        // The time since the last tick, and the cool down until the next one.
        TickDelta = TickFunction->LastTickGameTimeSeconds >= 0 && World
                        ? static_cast<float>(World->GetTimeSeconds() - TickFunction->LastTickGameTimeSeconds)
                        : DeltaSeconds;
        TickFunction->TickState = FTickFunction::ETickState::CoolingDown;
        TickFunction->RelativeTickCooldown = TickFunction->TickInterval;
    }
    if (World != nullptr) TickFunction->LastTickGameTimeSeconds = static_cast<float>(World->GetTimeSeconds());
    TickFunction->ExecuteTick(TickDelta, TickType);
}

void FTickTaskManager::RunTickGroup(ETickingGroup Group) {
    CurrentGroup = Group;
    // A copy: ticks register and unregister tick functions.
    const TArray<FTickFunction*> Functions = Scheduled[Group];
    for (FTickFunction* TickFunction : Functions)
        if (Scheduled[Group].Contains(TickFunction)) RunTickFunction(TickFunction, 0);
    for (int32 Iteration = 0; Iteration < 101 && NewlySpawned.Num() > 0; Iteration++) {
        const TArray<FTickFunction*> Spawned = NewlySpawned;
        NewlySpawned.Reset();
        CurrentGroup = TG_NewlySpawned;
        Scheduled[TG_NewlySpawned].Reset();
        for (FTickFunction* TickFunction : Spawned)
            if (CanRun(TickFunction)) Scheduled[TG_NewlySpawned].Add(TickFunction);
        for (FTickFunction* TickFunction : TArray<FTickFunction*>(Scheduled[TG_NewlySpawned]))
            if (Scheduled[TG_NewlySpawned].Contains(TickFunction)) RunTickFunction(TickFunction, 0);
        CurrentGroup = Group;
    }
    CompleteTickGroup(Group);
}

void FTickTaskManager::EndFrame() {
    while (PendingCompletions.Num() > 0) CompleteTickFunction(PendingCompletions[0]);
    bTickingFrame = false;
    NewlySpawned.Reset();
}

}
