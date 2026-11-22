#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodySetup.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

}

void UBodySetup::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << BodySetupGuid;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked) {
        if (Ar.GetGame() == EGame::GAME_Highguard || (Ar.GetGame() == EGame::GAME_Aion2 && bNeverNeedsCookedCollisionData))
            return;
        if (Ar.UEVer() >= V4::STORE_HASCOOKEDDATA_FOR_BODYSETUP)
            Ar << bHasCookedCollisionData;
        CookedFormatData.Serialize(Ar);
        if (Ar.GetGame() == EGame::GAME_DreamStar)
            Ar.SkipBytes(4);
    }
    if (Ar.CustomVer(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::LevelsetSerializationSupportForBodySetup ||
        Ar.CustomVer(FFortniteReleaseBranchCustomObjectVersion::GUID) >= FFortniteReleaseBranchCustomObjectVersion::DisableLevelset_v14_10)
        return;
    int32 NumImplicits = 0;
    Ar << NumImplicits;
    if (NumImplicits != 0)
        Ar.SetError(ErrorCode::UnsupportedVersion, "UBodySetup.ChaosImplicitObjects");
}

}
