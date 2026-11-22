#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NavAgentSelector.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNavAgentSelector {
    GENERATED_BODY()

    static constexpr uint32 InitializedBit = 0x80000000;
    static constexpr uint32 AllAgentsMask = 0x7fffffff;
    static constexpr int32 AgentsMaxCount = 31;

    union {
        struct {
            UPROPERTY()
            uint32 bSupportsAgent0 : 1;
            UPROPERTY()
            uint32 bSupportsAgent1 : 1;
            UPROPERTY()
            uint32 bSupportsAgent2 : 1;
            UPROPERTY()
            uint32 bSupportsAgent3 : 1;
            UPROPERTY()
            uint32 bSupportsAgent4 : 1;
            UPROPERTY()
            uint32 bSupportsAgent5 : 1;
            UPROPERTY()
            uint32 bSupportsAgent6 : 1;
            UPROPERTY()
            uint32 bSupportsAgent7 : 1;
            UPROPERTY()
            uint32 bSupportsAgent8 : 1;
            UPROPERTY()
            uint32 bSupportsAgent9 : 1;
            UPROPERTY()
            uint32 bSupportsAgent10 : 1;
            UPROPERTY()
            uint32 bSupportsAgent11 : 1;
            UPROPERTY()
            uint32 bSupportsAgent12 : 1;
            UPROPERTY()
            uint32 bSupportsAgent13 : 1;
            UPROPERTY()
            uint32 bSupportsAgent14 : 1;
            UPROPERTY()
            uint32 bSupportsAgent15 : 1;
            UPROPERTY()
            uint32 bSupportsAgent16 : 1;
            UPROPERTY()
            uint32 bSupportsAgent17 : 1;
            UPROPERTY()
            uint32 bSupportsAgent18 : 1;
            UPROPERTY()
            uint32 bSupportsAgent19 : 1;
            UPROPERTY()
            uint32 bSupportsAgent20 : 1;
            UPROPERTY()
            uint32 bSupportsAgent21 : 1;
            UPROPERTY()
            uint32 bSupportsAgent22 : 1;
            UPROPERTY()
            uint32 bSupportsAgent23 : 1;
            UPROPERTY()
            uint32 bSupportsAgent24 : 1;
            UPROPERTY()
            uint32 bSupportsAgent25 : 1;
            UPROPERTY()
            uint32 bSupportsAgent26 : 1;
            UPROPERTY()
            uint32 bSupportsAgent27 : 1;
            UPROPERTY()
            uint32 bSupportsAgent28 : 1;
            UPROPERTY()
            uint32 bSupportsAgent29 : 1;
            UPROPERTY()
            uint32 bSupportsAgent30 : 1;
        };
        uint32 PackedBits;
    };

    explicit FNavAgentSelector(const uint32 InBits = AllAgentsMask) : PackedBits(InBits) {}

    bool Contains(int32 AgentIndex) const { return AgentIndex >= 0 && AgentIndex < AgentsMaxCount && (PackedBits & (1u << AgentIndex)) != 0; }
    bool ContainsAnyAgent() const { return (PackedBits & AllAgentsMask) != 0; }
    void Set(int32 AgentIndex) {
        if (AgentIndex >= 0 && AgentIndex < AgentsMaxCount)
            PackedBits |= 1u << AgentIndex;
    }
    bool IsInitialized() const { return (PackedBits & InitializedBit) != 0; }
    void MarkInitialized() { PackedBits |= InitializedBit; }
    void Empty() { PackedBits = 0; }
    bool IsSame(const FNavAgentSelector& Other) const { return (~InitializedBit & PackedBits) == (~InitializedBit & Other.PackedBits); }
    uint32 GetAgentBits() const { return ~InitializedBit & PackedBits; }

    bool Serialize(FArchive& Ar) {
        Ar << PackedBits;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FNavAgentSelector> : TStructOpsTypeTraitsBase2<FNavAgentSelector> {
    enum { WithSerializer = true };
};

}
