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

UENUM()
enum class ESmartObjectTagMergingPolicy : uint8 { Combine, Override };

UENUM()
enum class ESmartObjectTagFilteringPolicy : uint8 { NoFilter, Combine, Override };

USTRUCT()
struct FSmartObjectDefinitionData {
    GENERATED_BODY()
};

USTRUCT()
struct FSmartObjectDefinitionDataHandle {
    GENERATED_BODY()

    static constexpr uint16 InvalidIndex = 0xFFFF;

    UPROPERTY()
    uint16 SlotIndex = InvalidIndex;
    UPROPERTY()
    uint16 DataIndex = InvalidIndex;
};

}
