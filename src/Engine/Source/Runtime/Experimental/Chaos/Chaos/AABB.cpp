#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

template <class T>
void SerializeAsAABB(FArchive& Ar, TAABB<T>& AABB) {
    if (!Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::TBoxReplacedWithTAABB, EGame::GAME_UE4_25)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "Chaos.SerializeAsAABB");
        return;
    }
    AABB.Serialize(Ar);
}

template void SerializeAsAABB(FArchive& Ar, TAABB<float>& AABB);
template void SerializeAsAABB(FArchive& Ar, TAABB<double>& AABB);

}
