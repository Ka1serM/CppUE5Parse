#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"
#include "ue5parse/runtime/Engine/Engine/LatentActionManager.h"

// The base of latent actions and what they answer each update (UE's Engine/Public/LatentActions.h).

namespace ue5parse::runtime {

// FLatentResponse: what an action asks for after an update: to finish, and the resume points to call.
class FLatentResponse {
public:
    explicit FLatentResponse(float InDeltaTime) : DeltaTime(InDeltaTime) {}

    FLatentResponse& DoneIf(bool Condition) {
        if (Condition) bRemoveAction = true;
        return *this;
    }
    FLatentResponse& TriggerLink(FName ExecutionFunction, int32 LinkID, const FWeakObjectPtr& CallbackTarget) {
        if (LinkID != INDEX_NONE) LinksToExecute.Add({ExecutionFunction, LinkID, CallbackTarget});
        return *this;
    }
    FLatentResponse& FinishAndTriggerIf(bool Condition, FName ExecutionFunction, int32 LinkID, const FWeakObjectPtr& CallbackTarget) {
        if (Condition) {
            bRemoveAction = true;
            TriggerLink(ExecutionFunction, LinkID, CallbackTarget);
        }
        return *this;
    }
    float ElapsedTime() const { return DeltaTime; }

private:
    friend class FLatentActionManager;
    struct FExecutionInfo {
        FName ExecutionFunction;
        int32 LinkID;
        FWeakObjectPtr CallbackTarget;
    };
    TArray<FExecutionInfo> LinksToExecute;
    bool bRemoveAction = false;
    float DeltaTime;
};

class FPendingLatentAction {
public:
    virtual ~FPendingLatentAction() = default;
    virtual void UpdateOperation(FLatentResponse& Response) { Response.DoneIf(true); }
    // The object or its world goes away; the action ends without resuming.
    virtual void NotifyObjectDestroyed() {}
    virtual void NotifyActionAborted() {}
};

// FDelayAction: resumes after the time (UKismetSystemLibrary::Delay).
class FDelayAction : public FPendingLatentAction {
public:
    FDelayAction(float Duration, const FLatentActionInfo& LatentInfo)
        : TimeRemaining(Duration), ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage),
          CallbackTarget(LatentInfo.CallbackTarget) {}
    void UpdateOperation(FLatentResponse& Response) override {
        TimeRemaining -= Response.ElapsedTime();
        Response.FinishAndTriggerIf(TimeRemaining <= 0.0f, ExecutionFunction, OutputLink, CallbackTarget);
    }

    float TimeRemaining;
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;
};

// FDelayUntilNextTickAction: resumes on the next update (UKismetSystemLibrary::DelayUntilNextTick).
class FDelayUntilNextTickAction : public FPendingLatentAction {
public:
    explicit FDelayUntilNextTickAction(const FLatentActionInfo& LatentInfo)
        : ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage), CallbackTarget(LatentInfo.CallbackTarget) {}
    void UpdateOperation(FLatentResponse& Response) override {
        Response.FinishAndTriggerIf(bCalledOnce, ExecutionFunction, OutputLink, CallbackTarget);
        bCalledOnce = true;
    }

    bool bCalledOnce = false;
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;
};

}
