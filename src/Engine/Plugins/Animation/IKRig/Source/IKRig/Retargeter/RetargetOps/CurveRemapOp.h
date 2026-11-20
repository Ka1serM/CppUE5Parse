#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"

#include "CurveRemapOp.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FCurveRemapPair {
    GENERATED_BODY()
    UPROPERTY()
    FName SourceCurve;
    UPROPERTY()
    FName TargetCurve;
};

USTRUCT()
struct FIKRetargetCurveRemapOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    bool bCopyAllSourceCurves = true;
    UPROPERTY()
    bool bRemapCurves = true;
    UPROPERTY()
    TArray<FCurveRemapPair> CurvesToRemap;
};

USTRUCT()
struct FIKRetargetCurveRemapOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetCurveRemapOpSettings Settings;
};

}
