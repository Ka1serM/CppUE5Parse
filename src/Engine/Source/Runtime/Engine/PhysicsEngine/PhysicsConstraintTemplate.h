#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConstraintInstance.h"

#include "PhysicsConstraintTemplate.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPhysicsConstraintProfileHandle {
    GENERATED_BODY()
    UPROPERTY()
    FConstraintProfileProperties ProfileProperties;
    UPROPERTY()
    FName ProfileName;
};

UCLASS()
class UPhysicsConstraintTemplate : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FConstraintInstance DefaultInstance;
    UPROPERTY()
    TArray<FPhysicsConstraintProfileHandle> ProfileHandles;
    UPROPERTY()
    FConstraintProfileProperties DefaultProfile;
};

}
