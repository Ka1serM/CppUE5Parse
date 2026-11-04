#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"

#include "LatentActionManager.generated.h"

namespace ue5parse::runtime {

class FPendingLatentAction;

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

class FLatentActionManager {
public:
    FLatentActionManager();
    ~FLatentActionManager();
    FLatentActionManager(const FLatentActionManager&) = delete;
    FLatentActionManager& operator=(const FLatentActionManager&) = delete;

    void ProcessLatentActions(UObject* InObject, float DeltaTime);
    template <class T>
    T* FindExistingAction(UObject* InActionObject, int32 UUID) {
        return static_cast<T*>(FindExistingActionInternal(InActionObject, UUID));
    }
    void AddNewAction(UObject* InActionObject, int32 UUID, std::unique_ptr<FPendingLatentAction> NewAction);
    void RemoveActionsForObject(UObject* InObject);
    void BeginFrame() { ProcessedThisFrame.Reset(); }

private:
    struct FObjectActions;
    FPendingLatentAction* FindExistingActionInternal(UObject* InActionObject, int32 UUID);
    void TickLatentActionForObject(float DeltaTime, FObjectActions& Actions, UObject* InObject);

    TMap<const UObject*, std::unique_ptr<FObjectActions>> ObjectToActionListMap;
    TArray<const UObject*> ProcessedThisFrame;
};

}
