#pragma once

#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAssetCommon.h"

#include "SkeletalMeshSourceModel.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSkeletalMeshSourceModel : public FSkeletalMeshLODInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 TriangleCount = 0;
    UPROPERTY()
    int32 VertexCount = 0;
    UPROPERTY()
    FBoxSphereBounds Bounds;
    UPROPERTY()
    TArray<FName> CachedSkinWeightProfileNames{NAME_None};
    UPROPERTY()
    TArray<FName> CachedMorphTargetNames{NAME_None};
    UPROPERTY()
    TOptional<bool> CachedHasVertexColors;
    UPROPERTY()
    FGuid CachedVertexColorGuid;
};

}
