#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "RigHierarchy.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URigHierarchy : public UObject {
    GENERATED_BODY()
public:
    FRigHierarchyData Hierarchy;

    void Serialize(FArchive& Ar) override;
};

}
