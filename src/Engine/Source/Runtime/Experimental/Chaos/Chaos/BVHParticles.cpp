#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/BVHParticles.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

template <class LEAF_TYPE, class T>
void TBoundingVolumeHierarchy<LEAF_TYPE, T>::Serialize(FArchive& Ar) {
    Ar << MGlobalObjects;
    if (!Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::TBoxReplacedWithTAABB, EGame::GAME_UE4_25)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "TBoundingVolumeHierarchy.SerializeAsAABBs");
        return;
    }
    Ar << MWorldSpaceBoxes << MMaxLevels << Elements << Leafs;
}

void FBVHParticles::Serialize(FChaosArchive& Ar) {
    bool bSerialize = true;
    Ar << bSerialize;
    if (!bSerialize) {
        Ar.SetError(ErrorCode::InvalidData, "FParticles.bSerialize");
        return;
    }
    MX.SetNum(static_cast<int32>(Ar.ReadCount(12)));
    for (FVector& X : MX)
        SerializeReal(Ar, X);
    MBVH.Serialize(Ar);
}

}
