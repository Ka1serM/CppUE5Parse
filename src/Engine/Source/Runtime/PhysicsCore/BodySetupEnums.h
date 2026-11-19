#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BodySetupEnums.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ECollisionTraceFlag : int32 { CTF_UseDefault, CTF_UseSimpleAndComplex, CTF_UseSimpleAsComplex, CTF_UseComplexAsSimple, CTF_MAX };

UENUM()
enum EPhysicsType : int32 { PhysType_Default, PhysType_Kinematic, PhysType_Simulated };

namespace EBodyCollisionResponse {

UENUM()
enum Type : int32 { BodyCollision_Enabled, BodyCollision_Disabled };

}

}
