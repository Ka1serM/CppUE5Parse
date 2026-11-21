#pragma once

#include <variant>

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyKey.h"

namespace ue5parse::runtime {

struct FRigMetadataEntry {
    FName ItemName;
    FName TypeName;
    FName Name;
    uint8 Type = 0;
    std::variant<
        bool, TArray<bool>,
        float, TArray<float>,
        int32, TArray<int32>,
        FName, TArray<FName>,
        FVector, TArray<FVector>,
        FRotator, TArray<FRotator>,
        FQuat, TArray<FQuat>,
        FTransform, TArray<FTransform>,
        FLinearColor, TArray<FLinearColor>,
        FRigElementKey, TArray<FRigElementKey>> Value;

    friend FArchive& operator<<(FArchive& Ar, FRigMetadataEntry& Entry);
};

}
