#pragma once

#include "ue5parse/runtime/Engine/Rendering/SkeletalMeshLODRenderData.h"

namespace ue5parse::runtime {

class FSkeletalMeshRenderData {
public:
    TArray<FSkeletalMeshLODRenderData> LODRenderData;
    uint8 NumInlinedLODs = 0;
    uint8 NumNonOptionalLODs = 0;

    void Serialize(FArchive& Ar, bool bHasVertexColors);
    int32 GetFirstValidLODIdx(int32 MinLODIdx) const;
};

}
