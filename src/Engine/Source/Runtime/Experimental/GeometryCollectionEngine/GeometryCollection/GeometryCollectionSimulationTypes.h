#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "GeometryCollectionSimulationTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ECollisionTypeEnum : uint8 { Chaos_Volumetric, Chaos_Surface_Volumetric, Chaos_Max };

UENUM()
enum class EImplicitTypeEnum : uint8 { Chaos_Implicit_Box, Chaos_Implicit_Sphere, Chaos_Implicit_Capsule, Chaos_Implicit_LevelSet, Chaos_Implicit_None, Chaos_Implicit_Convex, Chaos_Max };

UENUM()
enum class EObjectStateTypeEnum : uint8 {
    Chaos_NONE = 0,
    Chaos_Object_Sleeping = 1,
    Chaos_Object_Kinematic = 2,
    Chaos_Object_Static = 3,
    Chaos_Object_Dynamic = 4,
    Chaos_Object_UserDefined = 100,
    Chaos_Max
};

UENUM()
enum class EInitialVelocityTypeEnum : uint8 { Chaos_Initial_Velocity_User_Defined, Chaos_Initial_Velocity_None, Chaos_Max };

UENUM()
enum class EDamageModelTypeEnum : uint8 {
    Chaos_Damage_Model_UserDefined_Damage_Threshold,
    Chaos_Damage_Model_Material_Strength_And_Connectivity_DamageThreshold,
    Chaos_Max
};

}
