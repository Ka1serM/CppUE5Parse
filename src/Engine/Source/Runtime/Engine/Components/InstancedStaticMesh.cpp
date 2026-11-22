#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/PhysicsSerializer.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

namespace {

constexpr int32 LegacyUVBiasSize = 2 * 2 * sizeof(float);

//! Before UE 4.19 an instance also stores its lightmap and shadowmap UV bias as two FVector2D.
bool HasLegacyUVBias(const FArchive& Ar) {
    return !Ar.CustomVerAtLeast(FMobileObjectVersion::GUID, FMobileObjectVersion::InstancedStaticMeshLightmapSerialization, EGame::GAME_UE4_19);
}

void SerializeInstanceData(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances) {
    const bool bLegacyUVBias = HasLegacyUVBias(Ar);
    const int32 MatrixSize = 16 * (Ar.UEVer() < EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES ? sizeof(float) : sizeof(double));
    const int32 ExpectedElementSize = MatrixSize + (bLegacyUVBias ? LegacyUVBiasSize : 0);
    int32 ElementSize = 0;
    Ar << ElementSize;
    if (!Ar.IsError() && ElementSize != ExpectedElementSize) {
        Ar.SetError(ErrorCode::InvalidData, "BulkSerialize.ElementSize");
        return;
    }
    const int32 Count = Ar.ReadCount(ExpectedElementSize);
    Instances.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        Ar << Instances[Instances.AddDefaulted()].Transform;
        if (bLegacyUVBias)
            Ar.SkipBytes(LegacyUVBiasSize);
    }
}

void SkipInstanceBufferRenderData(FArchive& Ar) {
    uint64 RenderDataSizeBytes = 0;
    Ar << RenderDataSizeBytes;
    if (RenderDataSizeBytes > static_cast<uint64>(Ar.GetRemaining())) {
        Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.RenderData");
        return;
    }
    Ar.SkipBytes(static_cast<int64>(RenderDataSizeBytes));
}

//! UE 5.4 replaces the instance buffer with a precomputed spatial hash; UE 5.8 adds the primitive's uint32 local to world hash.
void SkipPrecomputedSpatialHashData(FArchive& Ar) {
    bool bHasCookedData = false;
    Ar << bHasCookedData;
    if (!bHasCookedData)
        return;
    if (Ar.GetGame() >= EGame::GAME_UE5_8)
        Ar.SkipBytes(sizeof(uint32));
    SkipBulkSerializedArray(Ar);
    SkipBulkSerializedArray(Ar);
}

void SkipCookedRenderData(FArchive& Ar) {
    if (Ar.GetGame() >= EGame::GAME_UE5_4)
        SkipPrecomputedSpatialHashData(Ar);
    else
        SkipInstanceBufferRenderData(Ar);
}

}

UInstancedStaticMeshComponent::UInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    const EGame Game = GetRuntimeProvider().getGame();
    if (Game >= EGame::GAME_UE4_8 && Game < EGame::GAME_UE4_20)
        PhysicsSerializer = ObjectInitializer.CreateDefaultSubobject<UPhysicsSerializer>(FName("PhysicsSerializer"));
}

void UInstancedStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    bool bCooked = false;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::SerializeInstancedStaticMeshRenderData, EGame::GAME_UE4_20)
        || Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::SerializeInstancedStaticMeshRenderData, EGame::GAME_UE4_20))
        Ar << bCooked;
    bool bHasPerInstanceData = true;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ISMComponentEditableWhenInheritedSkipSerialization,
                            EGame::GAME_UE5_4))
        Ar << bHasPerInstanceData;
    if (bHasPerInstanceData) {
        SerializeInstanceData(Ar, PerInstanceSMData);
        if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::PerInstanceCustomData, EGame::GAME_UE4_25))
            BulkSerialize(Ar, PerInstanceSMCustomData);
    } else if (!InheritPerInstanceData()) {
        Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.Archetype");
        return;
    }
    if (bCooked)
        SkipCookedRenderData(Ar);
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(4);
}

//! UE 5.4 leaves out the per-instance data of a component that inherits it from its blueprint archetype.
bool UInstancedStaticMeshComponent::InheritPerInstanceData() {
    const UInstancedStaticMeshComponent* Archetype = Cast<UInstancedStaticMeshComponent>(GetArchetype());
    if (Archetype == nullptr)
        return false;
    PerInstanceSMData = Archetype->PerInstanceSMData;
    PerInstanceSMCustomData = Archetype->PerInstanceSMCustomData;
    NumCustomDataFloats = Archetype->NumCustomDataFloats;
    return true;
}

bool UInstancedStaticMeshComponent::GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const {
    if (!PerInstanceSMData.IsValidIndex(InstanceIndex))
        return false;
    OutInstanceTransform = FTransform(PerInstanceSMData[InstanceIndex].Transform);
    return true;
}

}
