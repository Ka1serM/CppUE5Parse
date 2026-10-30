#pragma once

#include <memory>

#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"

#include "LatentActionManager.generated.h"

namespace ue5parse::runtime {

class FPendingLatentAction;

// FLatentActionInfo: where a latent Blueprint node resumes: the ubergraph function, the offset in it, and the node's id
// (UE's Engine/LatentActionManager.h).
USTRUCT()
struct FLatentActionInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 Linkage = INDEX_NONE;
    UPROPERTY()
    int32 UUID = INDEX_NONE;
    UPROPERTY()
    FName ExecutionFunction;
    UPROPERTY()
    TObjectPtr<UObject> CallbackTarget = nullptr;
};

// FLatentActionManager: the latent actions of each object (Delay, montage callbacks...), updated once a frame: an actor
// updates its own in its tick, the world the rest at the end of its tick (UE's FLatentActionManager).
class FLatentActionManager {
public:
    FLatentActionManager();
    ~FLatentActionManager();
    FLatentActionManager(const FLatentActionManager&) = delete;
    FLatentActionManager& operator=(const FLatentActionManager&) = delete;

    // Updates the actions of one object, or of every object not updated this frame for null.
    void ProcessLatentActions(UObject* InObject, float DeltaTime);
    // The action of an object by UUID, as T; null when it has none.
    template <class T>
    T* FindExistingAction(UObject* InActionObject, int32 UUID) {
        return static_cast<T*>(FindExistingActionInternal(InActionObject, UUID));
    }
    void AddNewAction(UObject* InActionObject, int32 UUID, std::unique_ptr<FPendingLatentAction> NewAction);
    void RemoveActionsForObject(UObject* InObject);
    int32 GetNumActionsForObject(UObject* InObject);
    // A new frame: every object may update again.
    void BeginFrame() { ProcessedThisFrame.Reset(); }

private:
    struct FObjectActions;
    FPendingLatentAction* FindExistingActionInternal(UObject* InActionObject, int32 UUID);
    void TickLatentActionForObject(float DeltaTime, FObjectActions& Actions, UObject* InObject);

    TMap<const UObject*, std::unique_ptr<FObjectActions>> ObjectToActionListMap;
    TArray<const UObject*> ProcessedThisFrame;
};

}
