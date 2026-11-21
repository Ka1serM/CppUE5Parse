#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "SmartObjectTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartObjectHandle {
    GENERATED_BODY()

    UPROPERTY()
    uint64 ID = 0;
    UPROPERTY()
    FGuid Guid;

    bool operator==(const FSmartObjectHandle& Other) const = default;
    friend uint32 GetTypeHash(const FSmartObjectHandle& Handle) { return HashCombine(GetTypeHash(Handle.ID), GetTypeHash(Handle.Guid)); }
};

}
