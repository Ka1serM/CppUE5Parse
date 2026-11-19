#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ChaosEngineInterface.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESleepFamily : uint8 { Normal, Sensitive, Custom };

UENUM()
enum EAngularConstraintMotion : int32 { ACM_Free, ACM_Limited, ACM_Locked, ACM_MAX };

UENUM()
enum ELinearConstraintMotion : int32 { LCM_Free, LCM_Limited, LCM_Locked, LCM_MAX };

UENUM()
enum EConstraintPlasticityType : int32 { CCPT_Free, CCPT_Shrink, CCPT_Grow, CCPT_MAX };

namespace EFrictionCombineMode {

UENUM()
enum Type : int32 { Average, Min, Multiply, Max };

}

UENUM()
enum EPhysicalSurface : int32 {
    SurfaceType_Default,
    SurfaceType1,
    SurfaceType2,
    SurfaceType3,
    SurfaceType4,
    SurfaceType5,
    SurfaceType6,
    SurfaceType7,
    SurfaceType8,
    SurfaceType9,
    SurfaceType10,
    SurfaceType11,
    SurfaceType12,
    SurfaceType13,
    SurfaceType14,
    SurfaceType15,
    SurfaceType16,
    SurfaceType17,
    SurfaceType18,
    SurfaceType19,
    SurfaceType20,
    SurfaceType21,
    SurfaceType22,
    SurfaceType23,
    SurfaceType24,
    SurfaceType25,
    SurfaceType26,
    SurfaceType27,
    SurfaceType28,
    SurfaceType29,
    SurfaceType30,
    SurfaceType31,
    SurfaceType32,
    SurfaceType33,
    SurfaceType34,
    SurfaceType35,
    SurfaceType36,
    SurfaceType37,
    SurfaceType38,
    SurfaceType39,
    SurfaceType40,
    SurfaceType41,
    SurfaceType42,
    SurfaceType43,
    SurfaceType44,
    SurfaceType45,
    SurfaceType46,
    SurfaceType47,
    SurfaceType48,
    SurfaceType49,
    SurfaceType50,
    SurfaceType51,
    SurfaceType52,
    SurfaceType53,
    SurfaceType54,
    SurfaceType55,
    SurfaceType56,
    SurfaceType57,
    SurfaceType58,
    SurfaceType59,
    SurfaceType60,
    SurfaceType61,
    SurfaceType62,
    SurfaceType_Max,
};

}
