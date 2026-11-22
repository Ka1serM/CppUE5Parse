#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"

#include "NetDriver.generated.h"

namespace ue5parse::runtime {

class UNetConnection;

UCLASS(Abstract, Transient)
class UNetDriver : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UNetConnection> ServerConnection = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UNetConnection>> ClientConnections;

    ENetMode GetNetMode() const;
    virtual bool IsServer() const;
    bool IsInTick() const { return bInTick; }
    void SetIsInTick(bool bIsInTick) { bInTick = bIsInTick; }

private:
    bool bInTick = false;
};

}
