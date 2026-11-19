#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/PhysicsCore/BodySetupEnums.h"

#include "BodySetupCore.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBodySetupCore : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    TEnumAsByte<EPhysicsType> PhysicsType = PhysType_Default;
    UPROPERTY()
    TEnumAsByte<ECollisionTraceFlag> CollisionTraceFlag = CTF_UseDefault;
    UPROPERTY()
    TEnumAsByte<EBodyCollisionResponse::Type> CollisionReponse = EBodyCollisionResponse::BodyCollision_Enabled;
};

}
