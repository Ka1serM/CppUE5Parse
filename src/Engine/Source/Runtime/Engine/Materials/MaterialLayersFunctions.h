#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialLayersFunctions.generated.h"

namespace ue5parse::runtime {

class UMaterialFunctionInterface;

UENUM()
enum class EMaterialLayerLinkState : uint8 { Uninitialized, LinkedToParent, UnlinkedFromParent, NotFromParent };

USTRUCT()
struct FMaterialLayersFunctionsTree {
    GENERATED_BODY()
    using FNodeId = int32;
    using FPayloadId = int32;
    static constexpr FNodeId InvalidId = -1;

    struct FNode {
        FNodeId Parent = InvalidId;
        FNodeId NextSibling = InvalidId;
        FNodeId ChildrenHead = InvalidId;
        FNodeId Spare = InvalidId;
        friend FArchive& operator<<(FArchive& Ar, FNode& Node) { return Ar << Node.Parent << Node.NextSibling << Node.ChildrenHead << Node.Spare; }
    };

    struct FPayload {
        FPayloadId Layer = InvalidId;
        FPayloadId Blend = InvalidId;
        friend FArchive& operator<<(FArchive& Ar, FPayload& Payload) { return Ar << Payload.Layer << Payload.Blend; }
    };

    bool Serialize(FArchive& Ar) {
        Ar << Nodes << Payloads << Root;
        return true;
    }

    TArray<FNode> Nodes;
    TArray<FPayload> Payloads;
    FNodeId Root = InvalidId;
};

template <>
struct TStructOpsTypeTraits<FMaterialLayersFunctionsTree> : TStructOpsTypeTraitsBase2<FMaterialLayersFunctionsTree> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMaterialLayersFunctionsRuntimeData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<UMaterialFunctionInterface>> Layers;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialFunctionInterface>> Blends;
    UPROPERTY()
    FMaterialLayersFunctionsTree Tree;
};

USTRUCT()
struct FMaterialLayersFunctions : public FMaterialLayersFunctionsRuntimeData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<bool> LayerStates;
    UPROPERTY()
    FString KeyString;
    UPROPERTY()
    TArray<FText> LayerNames_DEPRECATED;
    UPROPERTY()
    TArray<bool> RestrictToLayerRelatives_DEPRECATED;
    UPROPERTY()
    TArray<bool> RestrictToBlendRelatives_DEPRECATED;
    UPROPERTY()
    TArray<FGuid> LayerGuids_DEPRECATED;
    UPROPERTY()
    TArray<EMaterialLayerLinkState> LayerLinkStates_DEPRECATED;
    UPROPERTY()
    TArray<FGuid> DeletedParentLayerGuids_DEPRECATED;
};

}
