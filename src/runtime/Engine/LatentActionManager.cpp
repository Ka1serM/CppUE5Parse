#include "ue5parse/runtime/Engine/Engine/LatentActionManager.h"

#include "ue5parse/runtime/Engine/LatentActions.h"

// UE's LatentActionManager.cpp (UE 5.3).

namespace ue5parse::runtime {

struct FLatentActionManager::FObjectActions {
    FWeakObjectPtr Object;
    // By UUID, in the order the actions were added.
    TArray<std::pair<int32, std::unique_ptr<FPendingLatentAction>>> Actions;
};

FLatentActionManager::FLatentActionManager() = default;
FLatentActionManager::~FLatentActionManager() = default;

FPendingLatentAction* FLatentActionManager::FindExistingActionInternal(UObject* InActionObject, int32 UUID) {
    std::unique_ptr<FObjectActions>* Found = ObjectToActionListMap.Find(InActionObject);
    if (Found == nullptr) return nullptr;
    for (auto& [ActionUUID, Action] : (*Found)->Actions)
        if (ActionUUID == UUID) return Action.get();
    return nullptr;
}

void FLatentActionManager::AddNewAction(UObject* InActionObject, int32 UUID, std::unique_ptr<FPendingLatentAction> NewAction) {
    std::unique_ptr<FObjectActions>& Actions = ObjectToActionListMap.FindOrAdd(InActionObject);
    if (!Actions) {
        Actions = std::make_unique<FObjectActions>();
        Actions->Object = InActionObject;
    }
    Actions->Actions.Add({UUID, std::move(NewAction)});
}

void FLatentActionManager::RemoveActionsForObject(UObject* InObject) {
    std::unique_ptr<FObjectActions>* Found = ObjectToActionListMap.Find(InObject);
    if (Found == nullptr) return;
    for (auto& [UUID, Action] : (*Found)->Actions) Action->NotifyActionAborted();
    (*Found)->Actions.Empty();
}

int32 FLatentActionManager::GetNumActionsForObject(UObject* InObject) {
    std::unique_ptr<FObjectActions>* Found = ObjectToActionListMap.Find(InObject);
    return Found ? (*Found)->Actions.Num() : 0;
}

void FLatentActionManager::TickLatentActionForObject(float DeltaTime, FObjectActions& Actions, UObject* InObject) {
    // The actions run in order; the ones that finish are removed and their resume points called after the update.
    TArray<FLatentResponse::FExecutionInfo> Links;
    for (int32 Index = 0; Index < Actions.Actions.Num();) {
        FLatentResponse Response(DeltaTime);
        Actions.Actions[Index].second->UpdateOperation(Response);
        for (const auto& Link : Response.LinksToExecute) Links.Add(Link);
        if (Response.bRemoveAction) Actions.Actions.RemoveAt(Index);
        else Index++;
    }
    for (const FLatentResponse::FExecutionInfo& Link : Links) {
        UObject* Target = Link.CallbackTarget.Get();
        if (Target == nullptr) continue;
        UFunction* ExecutionFunction = Target->FindFunction(Link.ExecutionFunction);
        if (ExecutionFunction == nullptr) continue;
        int32 Linkage = Link.LinkID;
        Target->ProcessEvent(ExecutionFunction, &Linkage);
    }
}

void FLatentActionManager::ProcessLatentActions(UObject* InObject, float DeltaTime) {
    if (InObject != nullptr) {
        if (ProcessedThisFrame.Contains(InObject)) return;
        ProcessedThisFrame.Add(InObject);
        std::unique_ptr<FObjectActions>* Found = ObjectToActionListMap.Find(InObject);
        if (Found != nullptr) TickLatentActionForObject(DeltaTime, **Found, InObject);
        return;
    }
    // Every object not updated this frame; objects that are gone drop their actions.
    TArray<const UObject*> Objects;
    for (const auto& Pair : ObjectToActionListMap) Objects.Add(Pair.Key);
    for (const UObject* Key : Objects) {
        std::unique_ptr<FObjectActions>* Found = ObjectToActionListMap.Find(Key);
        if (Found == nullptr) continue;
        UObject* Object = (*Found)->Object.Get();
        if (Object == nullptr) {
            for (auto& [UUID, Action] : (*Found)->Actions) Action->NotifyObjectDestroyed();
            ObjectToActionListMap.Remove(Key);
            continue;
        }
        if (ProcessedThisFrame.Contains(Object)) continue;
        ProcessedThisFrame.Add(Object);
        TickLatentActionForObject(DeltaTime, **Found, Object);
    }
}

}
