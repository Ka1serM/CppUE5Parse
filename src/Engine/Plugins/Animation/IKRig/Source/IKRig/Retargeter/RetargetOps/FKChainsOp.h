#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetChainMapping.h"
#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargetOps.h"

#include "FKChainsOp.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EFKChainTranslationMode : uint8 {
    None,
    GloballyScaled,
    Absolute,
    StretchBoneLengthUniformly,
    StretchBoneLengthNonUniformly,
    OrientAndScale,
};

UENUM()
enum class EFKChainRotationMode : uint8 {
    None = 5,
    Interpolated = 0,
    OneToOne = 1,
    OneToOneReversed = 2,
    MatchChain = 3,
    MatchScaledChain = 4,
    CopyLocal = 6,
};

USTRUCT()
struct FRetargetFKChainSettings {
    GENERATED_BODY()
    UPROPERTY()
    FName TargetChainName;
    UPROPERTY()
    bool EnableFK = true;
    UPROPERTY()
    EFKChainRotationMode RotationMode = EFKChainRotationMode::Interpolated;
    UPROPERTY()
    double RotationAlpha = 1.0;
    UPROPERTY()
    EFKChainTranslationMode TranslationMode = EFKChainTranslationMode::None;
    UPROPERTY()
    double TranslationAlpha = 1.0;
};

USTRUCT()
struct FIKRetargetFKChainsOpSettings : public FIKRetargetOpSettingsBase {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UIKRigDefinition> IKRigAsset = nullptr;
    UPROPERTY()
    TArray<FRetargetFKChainSettings> ChainsToRetarget;
    UPROPERTY()
    bool bDrawChainLines = true;
    UPROPERTY()
    bool bDrawSingleBoneChains = true;
    UPROPERTY()
    double ChainDrawThickness = 1.0;
    UPROPERTY()
    double ChainDrawSize = 1.0;
    UPROPERTY()
    FRetargetChainMapping ChainMapping;
};

USTRUCT()
struct FIKRetargetFKChainsOp : public FIKRetargetOpBase {
    GENERATED_BODY()
    UPROPERTY()
    FIKRetargetFKChainsOpSettings Settings;
    UPROPERTY()
    FRetargetChainMapping ChainMapping_DEPRECATED;
};

}
