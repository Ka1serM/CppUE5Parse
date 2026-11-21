#include "Engine/Source/Runtime/Engine/Components/InstancedSkinnedMeshComponent.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UInstancedSkinnedMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    bool bCooked = false;
    bool bHasSkipSerializationPropertiesData = false;
    Ar << bCooked << bHasSkipSerializationPropertiesData;
    const bool bInstanceDataV2 = Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::SkinnedMeshInstanceDataSerializationV2, EGame::GAME_UE5_6);
    if (bHasSkipSerializationPropertiesData) {
        if (bInstanceDataV2) {
            Ar << InstanceData;
        } else {
            TArray<FSkinnedMeshInstanceData_Deprecated> InstanceData_Deprecated;
            BulkSerialize(Ar, InstanceData_Deprecated);
            InstanceData.Empty(InstanceData_Deprecated.Num());
            for (const FSkinnedMeshInstanceData_Deprecated& Item : InstanceData_Deprecated)
                InstanceData.Add(FSkinnedMeshInstanceData{.Transform = FTransform3f(Item.Transform), .AnimationIndex = Item.BankIndex});
        }
        BulkSerialize(Ar, InstanceCustomData);
    }
    if (bInstanceDataV2)
        Ar << IndexToIdMap << NumInstances << MaxInstanceId;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::InstancedSkinnedMeshBoneAttachments, EGame::GAME_UE5_8)) {
        int32 NumSockets = 0;
        Ar << NumSockets;
        BoneAttachmentSockets.SetNum(NumSockets);
        for (FBoneAttachmentSocketReference& Socket : BoneAttachmentSockets)
            Ar << Socket.Parent << Socket.SocketName << Socket.SavedIndex;
        Ar << BoneAttachmentBindings;
    }
    if (!bCooked)
        return;
    bool bHasCookedData = false;
    Ar << bHasCookedData;
    if (!bHasCookedData)
        return;
    BulkSerialize(Ar, Hashes);
    BulkSerialize(Ar, ProxyIndexToComponentIndexRemap);
}

}
