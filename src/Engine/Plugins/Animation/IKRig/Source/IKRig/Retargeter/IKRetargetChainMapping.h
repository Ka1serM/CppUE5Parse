#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Rig/IKRigDefinition.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "IKRetargetChainMapping.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRetargetChainPair {
    GENERATED_BODY()
    UPROPERTY()
    FName TargetChainName;
    UPROPERTY()
    FName SourceChainName;
};

USTRUCT()
struct FRetargetChainMapping {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRetargetChainPair> ChainMap;
    UPROPERTY()
    TObjectPtr<const UIKRigDefinition> SourceIKRig = nullptr;
    UPROPERTY()
    TObjectPtr<const UIKRigDefinition> TargetIKRig = nullptr;
};

}
