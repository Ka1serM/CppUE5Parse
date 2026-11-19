#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "NaniteAssemblyData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNaniteAssemblyBoneInfluence {
    GENERATED_BODY()
    UPROPERTY()
    int32 BoneIndex = INDEX_NONE;
    UPROPERTY()
    float BoneWeight = 1.0f;
};

UENUM()
enum class ENaniteAssemblyNodeTransformSpace : uint8 { Local, BoneRelative };

USTRUCT()
struct FNaniteAssemblyNode {
    GENERATED_BODY()
    UPROPERTY()
    int32 ParentIndex = INDEX_NONE;
    UPROPERTY()
    int32 PartIndex = INDEX_NONE;
    UPROPERTY()
    ENaniteAssemblyNodeTransformSpace TransformSpace = ENaniteAssemblyNodeTransformSpace::Local;
    UPROPERTY()
    FTransform3f Transform;
    UPROPERTY()
    TArray<FNaniteAssemblyBoneInfluence> BoneInfluences;
};

USTRUCT()
struct FNaniteAssemblyPart {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath MeshObjectPath;
    UPROPERTY()
    TArray<int32> MaterialRemap;
};

USTRUCT()
struct FNaniteAssemblyData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FNaniteAssemblyPart> Parts;
    UPROPERTY()
    TArray<FNaniteAssemblyNode> Nodes;
};

}
