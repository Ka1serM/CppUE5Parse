#include "ue5parse/runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "ue5parse/provider/FileProvider.h"

// UE's Engine/InstancedStaticMesh.cpp (UE 5.3).

namespace ue5parse::runtime {

void UInstancedStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd()) return;
    bool bCooked = false;
    Ar << bCooked;
    BulkSerialize(Ar, PerInstanceSMData);
    BulkSerialize(Ar, PerInstanceSMCustomData);
    if (bCooked) {
        // The cooked instance render data: nothing reads it.
        uint64 RenderDataSizeBytes = 0;
        Ar << RenderDataSizeBytes;
        if (RenderDataSizeBytes > static_cast<uint64>(Ar.GetRemaining())) {
            Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.RenderData");
            return;
        }
        Ar.SkipBytes(static_cast<int64>(RenderDataSizeBytes));
    }
    if (Ar.HasGameQuirk(QUIRK_InstancedStaticMeshPadding)) Ar.SkipBytes(4);
}

bool UInstancedStaticMeshComponent::GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const {
    if (!PerInstanceSMData.IsValidIndex(InstanceIndex)) return false;
    OutInstanceTransform = FTransform(PerInstanceSMData[InstanceIndex].Transform);
    return true;
}

}
