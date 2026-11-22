#include "Engine/Source/Runtime/Landscape/LandscapeHeightfieldCollisionComponent.h"

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

namespace ue5parse::runtime {

ULandscapeHeightfieldCollisionComponent::ULandscapeHeightfieldCollisionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("BlockAll");
    bGenerateOverlapEvents = false;
    CastShadow = false;
    bUseAsOccluder = true;
    bAllowCullDistanceVolume = false;
    Mobility = EComponentMobility::Static;
    bCanEverAffectNavigation = true;
    bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
}

void ULandscapeHeightfieldCollisionComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(16);
    if (Ar.IsError() || Ar.UEVer() < EUnrealEngineObjectUE4Version::LANDSCAPE_COLLISION_DATA_COOKING)
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (!bCooked)
        return;
    if (Ar.GetGame() == EGame::GAME_PlayerUnknownsBattlegrounds) {
        FByteBulkData CookedCollisionBulkData;
        CookedCollisionBulkData.Serialize(Ar);
        return;
    }
    if (Ar.GetGame() >= EGame::GAME_UE4_14) {
        BulkSerialize(Ar, CookedCollisionData);
        return;
    }
    Ar << CookedCollisionData;
    //! Before UE 4.7 the cooked physical materials are native data, not a property.
    if (Ar.GetGame() < EGame::GAME_UE4_7)
        Ar << CookedPhysicalMaterials;
}

}
