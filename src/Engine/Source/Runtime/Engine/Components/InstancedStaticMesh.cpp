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

int32 GetMatrixSize(const FArchive& Ar) {
    return 16 * (Ar.UEVer() < EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES ? sizeof(float) : sizeof(double));
}

//! Some games store more data than the transform in each instance.
int32 GetInstanceTrailingSize(const FArchive& Ar) {
    switch (Ar.GetGame()) {
    case EGame::GAME_AWayOut:
    case EGame::GAME_PlayerUnknownsBattlegrounds:
    case EGame::GAME_SeaOfThieves:
    case EGame::GAME_AceCombat7:
    case EGame::GAME_DaysGone:
    case EGame::GAME_InfinityNikki:
    case EGame::GAME_NarutotoBorutoShinobiStriker:
    case EGame::GAME_DragonQuestXI:
    case EGame::GAME_WeHappyFew:
    case EGame::GAME_CodeVein:
    case EGame::GAME_TheDivisionResurgence:
        return LegacyUVBiasSize;
    case EGame::GAME_SilentHill2Remake:
    case EGame::GAME_StateOfDecay2:
    case EGame::GAME_ThePathless:
    case EGame::GAME_Abzu:
        return 2 * LegacyUVBiasSize;
    default:
        return HasLegacyUVBias(Ar) ? LegacyUVBiasSize : 0;
    }
}

void SerializeInstance(FArchive& Ar, FInstancedStaticMeshInstanceData& Instance, int32 ExtraSize) {
    Ar << Instance.Transform;
    if (Ar.GetGame() == EGame::GAME_HogwartsLegacy) {
        int32 Count = 0;
        Ar << Count;
        Ar.SkipBytes(static_cast<int64>(Count) * 4 + 4);
        return;
    }
    Ar.SkipBytes(GetInstanceTrailingSize(Ar) + ExtraSize);
}

void SerializeInstances(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances, int32 Count, int32 ExtraSize) {
    Instances.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        SerializeInstance(Ar, Instances[Instances.AddDefaulted()], ExtraSize);
}

//! Hogwarts Legacy instances have a variable size, so the bulk element size is not checked.
void SerializeInstanceData(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances, int32 ExtraSize = 0) {
    const int32 ExpectedElementSize = GetMatrixSize(Ar) + GetInstanceTrailingSize(Ar) + ExtraSize;
    int32 ElementSize = 0;
    Ar << ElementSize;
    if (!Ar.IsError() && ElementSize != ExpectedElementSize && Ar.GetGame() != EGame::GAME_HogwartsLegacy) {
        Ar.SetError(ErrorCode::InvalidData, "BulkSerialize.ElementSize");
        return;
    }
    SerializeInstances(Ar, Instances, Ar.ReadCount(ExpectedElementSize), ExtraSize);
}

//! Throne and Liberty picks the instance layout from the bulk element size.
void SerializeThroneAndLibertyInstanceData(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances) {
    int32 ElementSize = 0;
    int32 ElementCount = 0;
    Ar << ElementSize << ElementCount;
    Ar.Seek(Ar.Tell() - 2 * sizeof(int32));
    switch (ElementSize) {
    case 16:
        SkipBulkSerializedArray(Ar);
        if (ElementCount > 0)
            Ar.SkipBytes(24);
        return;
    case 40:
        SerializeInstances(Ar, Instances, Ar.ReadCount(GetMatrixSize(Ar)), 0);
        return;
    case 64:
        SkipBulkSerializedArray(Ar);
        return;
    default:
        Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.ThroneAndLiberty");
    }
}

}

//! Reads the per-instance transforms, with the layout of each game.
void SerializePerInstanceData(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances) {
    switch (Ar.GetGame()) {
    case EGame::GAME_Stalker2:
        Ar.SkipBytes(4);
        SerializeInstances(Ar, Instances, Ar.ReadCount(128), 0);
        return;
    case EGame::GAME_ThroneAndLiberty:
        SerializeThroneAndLibertyInstanceData(Ar, Instances);
        return;
    case EGame::GAME_PlayerUnknownsBattlegrounds: {
        int32 ElementSize = 0;
        Ar << ElementSize;
        Ar.Seek(Ar.Tell() - sizeof(int32));
        //! PUBG instances of 100 bytes have 20 more bytes after the UV bias.
        SerializeInstanceData(Ar, Instances, ElementSize == 100 ? 20 : 0);
        return;
    }
    default:
        SerializeInstanceData(Ar, Instances);
    }
}

namespace {

bool ReadFlag(FArchive& Ar) {
    uint8 Value = 0;
    Ar << Value;
    if (Value > 1)
        Ar.SetError(ErrorCode::InvalidData, "ReadFlag");
    return Value == 1;
}

//! Wuthering Waves can replace the instance data with its own layout.
bool SerializeWutheringWavesInstanceData(FArchive& Ar, TArray<FInstancedStaticMeshInstanceData>& Instances, TArray<float>& CustomData) {
    if (!ReadFlag(Ar))
        return false;
    Ar.SkipBytes(12);
    const int32 Count = Ar.ReadCount(13);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        Ar.SkipBytes(12);
        Ar.SkipBytes(ReadFlag(Ar) ? 3 : 7);
    }
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    BulkSerialize(Ar, CustomData);
    int64 Size = 0;
    Ar << Size;
    Ar.SkipBytes(Size + 8);
    SerializeInstanceData(Ar, Instances);
    return true;
}

//! MotoGP 24 stores packed instance data as arrays of FVector4.
void SkipMotoGP24InstanceData(FArchive& Ar) {
    int32 ElementSize = 0;
    Ar << ElementSize;
    const int32 Count = Ar.ReadCount(1);
    Ar.SkipBytes(static_cast<int64>(Count) * (ElementSize / 16) * 16);
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
    if (Ar.GetGame() >= EGame::GAME_UE5_4) {
        SkipPrecomputedSpatialHashData(Ar);
        return;
    }
    if (Ar.GetGame() == EGame::GAME_AssaultFireFuture)
        SkipBulkSerializedArray(Ar);
    SkipInstanceBufferRenderData(Ar);
    if (Ar.GetGame() == EGame::GAME_Lego2KDrive)
        SkipBulkSerializedArray(Ar);
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
    if (Ar.GetGame() == EGame::GAME_WutheringWaves && SerializeWutheringWavesInstanceData(Ar, PerInstanceSMData, PerInstanceSMCustomData))
        return;
    bool bHasPerInstanceData = true;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ISMComponentEditableWhenInheritedSkipSerialization,
                            EGame::GAME_UE5_4))
        Ar << bHasPerInstanceData;
    if (Ar.GetGame() == EGame::GAME_HonorofKingsWorld) {
        bool bCustomGameData = false;
        Ar << bCustomGameData << bHasPerInstanceData;
    }
    if (bHasPerInstanceData) {
        SerializePerInstanceData(Ar, PerInstanceSMData);
        if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::PerInstanceCustomData, EGame::GAME_UE4_25)
            || Ar.GetGame() == EGame::GAME_DeltaForce)
            BulkSerialize(Ar, PerInstanceSMCustomData);
    } else if (Ar.GetGame() != EGame::GAME_HonorofKingsWorld && !InheritPerInstanceData()) {
        Ar.SetError(ErrorCode::InvalidData, "UInstancedStaticMeshComponent.Archetype");
        return;
    }
    if (Ar.GetGame() == EGame::GAME_MotoGP24)
        SkipMotoGP24InstanceData(Ar);
    if (Ar.GetGame() == EGame::GAME_SuicideSquad)
        SkipBulkSerializedArray(Ar);
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
