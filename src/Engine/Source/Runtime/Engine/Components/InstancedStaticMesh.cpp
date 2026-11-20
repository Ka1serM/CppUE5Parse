#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

namespace {

void SerializeRenderData(FArchive& Ar) {
    bool bHasCookedData = false;
    Ar << bHasCookedData;
    if (!bHasCookedData)
        return;
    if (Ar.GetGame() >= EGame::GAME_UE5_8)
        Ar.SkipBytes(sizeof(uint32));
    SkipBulkSerializedArray(Ar);
    SkipBulkSerializedArray(Ar);
}

}

void UInstancedStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    bool bCooked = false;
    Ar << bCooked;
    bool bHasSkipSerializationPropertiesData = true;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ISMComponentEditableWhenInheritedSkipSerialization,
                            EGame::GAME_UE5_4))
        Ar << bHasSkipSerializationPropertiesData;
    if (bHasSkipSerializationPropertiesData) {
        BulkSerialize(Ar, PerInstanceSMData);
        BulkSerialize(Ar, PerInstanceSMCustomData);
    }
    if (bCooked && Ar.GetGame() >= EGame::GAME_UE5_4) {
        SerializeRenderData(Ar);
        return;
    }
    if (bCooked) {
        uint64 RenderDataSizeBytes = 0;
        Ar << RenderDataSizeBytes;
        if (RenderDataSizeBytes > static_cast<uint64>(Ar.GetRemaining())) {
            Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.RenderData");
            return;
        }
        Ar.SkipBytes(static_cast<int64>(RenderDataSizeBytes));
    }
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(4);
}

bool UInstancedStaticMeshComponent::GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const {
    if (!PerInstanceSMData.IsValidIndex(InstanceIndex))
        return false;
    OutInstanceTransform = FTransform(PerInstanceSMData[InstanceIndex].Transform);
    return true;
}

}
