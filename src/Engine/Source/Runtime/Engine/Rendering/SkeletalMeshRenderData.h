#pragma once

#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshLODRenderData.h"

namespace ue5parse::runtime {

class FSkeletalMeshRenderData {
public:
    TArray<FSkeletalMeshLODRenderData> LODRenderData;
    uint8 NumInlinedLODs = 0;
    uint8 NumNonOptionalLODs = 0;

    void Serialize(FArchive& Ar, bool bHasVertexColors, uint8 NumVertexColorChannels = 0);
    void SerializeGameForPeace(FArchive& Ar, bool bHasVertexColors, bool bIsStreamable, bool bHasSkinWeightProfiles);
};

}
