#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsAsset.h"

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/SkeletalBodySetup.h"

namespace ue5parse::runtime {

void UPhysicsAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    int32 NumPairs = 0;
    Ar << NumPairs;
    for (int32 Index = 0; Index < NumPairs && !Ar.IsError(); Index++) {
        FRigidBodyIndexPair Pair;
        bool bDisabled = false;
        Ar << Pair << bDisabled;
        CollisionDisableTable.Add(Pair, bDisabled);
    }
}

}
