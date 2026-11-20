#include "Engine/Source/Runtime/Foliage/InstancedFoliage.h"

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Foliage/InstancedFoliageActor.h"

namespace ue5parse::runtime {

namespace {

struct FFoliageCustomVersion {
    static constexpr FGuid GUID{0x430C4D19, 0x71544970, 0x87699B69, 0xDF90B0E5};
    enum Type : int32 {
        CrossLevelBase = 5,
        FoliageActorSupport = 12,
    };
};

void SerializeFoliageMeshInfos(FArchive& Ar, TMap<TObjectPtr<UFoliageType>, FFoliageInfo>& FoliageInfos) {
    const int32 Count = Ar.ReadCount(1);
    FoliageInfos.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        TObjectPtr<UFoliageType> FoliageType;
        FFoliageStaticMesh StaticMesh;
        Ar << FoliageType << StaticMesh.Component;
        FoliageInfos.Add(FoliageType, FFoliageInfo{EFoliageImplType::StaticMesh, StaticMesh});
    }
}

}

FArchive& operator<<(FArchive& Ar, FFoliageInfo& Info) {
    uint8 Type = 0;
    Ar << Type;
    Info.Type = static_cast<EFoliageImplType>(Type);
    switch (Info.Type) {
    case EFoliageImplType::StaticMesh: {
        FFoliageStaticMesh StaticMesh;
        Ar << StaticMesh.Component;
        Info.Implementation = StaticMesh;
        return Ar;
    }
    case EFoliageImplType::Actor: {
        FFoliageActor Actor;
        Ar << Actor.ActorInstances << Actor.ActorClass;
        Info.Implementation = std::move(Actor);
        return Ar;
    }
    default:
        Ar.SetError(ErrorCode::InvalidData, "FFoliageInfo.Type");
        return Ar;
    }
}

AInstancedFoliageActor::AInstancedFoliageActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = false;
}

//! Before FoliageActorSupport UE 4 stores each foliage type with the component of its FFoliageMeshInfo; UE 5 converts it to a static mesh FFoliageInfo.
void AInstancedFoliageActor::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() < EUnrealEngineObjectUE4Version::FOLIAGE_SETTINGS_TYPE
        || !Ar.CustomVerAtLeast(FFoliageCustomVersion::GUID, FFoliageCustomVersion::CrossLevelBase, EGame::GAME_UE4_8))
        return;
    if (Ar.CustomVerAtLeast(FFoliageCustomVersion::GUID, FFoliageCustomVersion::FoliageActorSupport, EGame::GAME_UE4_23))
        Ar << FoliageInfos;
    else
        SerializeFoliageMeshInfos(Ar, FoliageInfos);
}

}
