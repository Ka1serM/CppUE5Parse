#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyArchive.h"

namespace ue5parse::runtime {

struct FRigComputedTransform {
    FTransform Transform;
    bool bDirty = false;

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
};

struct FRigLocalAndGlobalTransform {
    FRigComputedTransform Local;
    FRigComputedTransform Global;

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
};

struct FRigCurrentAndInitialTransform {
    FRigLocalAndGlobalTransform Current;
    FRigLocalAndGlobalTransform Initial;

    void Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
};

}
