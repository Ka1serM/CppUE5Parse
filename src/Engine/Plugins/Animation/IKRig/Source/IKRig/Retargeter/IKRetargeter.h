#pragma once

#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Rig/IKRigDefinition.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Math/Quat.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "IKRetargeter.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIKRetargetPose {
    GENERATED_BODY()
    UPROPERTY()
    FVector RootTranslationOffset = FVector::ZeroVector;
    UPROPERTY()
    TMap<FName, FQuat> BoneRotationOffsets;
};

UCLASS()
class URetargetChainSettings : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SourceChain;
    UPROPERTY()
    FName TargetChain;
};

UCLASS()
class URetargetRootSettings : public UObject {
    GENERATED_BODY()
};

UCLASS()
class UIKRetargetGlobalSettings : public UObject {
    GENERATED_BODY()
};

UCLASS()
class URetargetOpStack : public UObject {
    GENERATED_BODY()
};

UCLASS()
class UIKRetargeter : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 Version = INDEX_NONE;
    //! UE 5.1 to 5.5 save the rig assets as soft references.
    UPROPERTY()
    TObjectPtr<UIKRigDefinition> SourceIKRigAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UIKRigDefinition> TargetIKRigAsset = nullptr;
    //! UE 5.0 saves the retarget phases outside the editor data.
    UPROPERTY()
    bool bRetargetRoot = true;
    UPROPERTY()
    bool bRetargetFK = true;
    UPROPERTY()
    bool bRetargetIK = true;
    //! UE 5.0 to 5.5 save the chain settings here; UE 5.6 moves them to the retarget ops.
    UPROPERTY()
    TArray<TObjectPtr<URetargetChainSettings>> ChainSettings;
    UPROPERTY()
    TObjectPtr<URetargetRootSettings> RootSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UIKRetargetGlobalSettings> GlobalSettings = nullptr;
    UPROPERTY()
    TObjectPtr<URetargetOpStack> OpStack = nullptr;
    UPROPERTY()
    FName CurrentProfile;
    UPROPERTY()
    TMap<FName, FIKRetargetPose> SourceRetargetPoses;
    UPROPERTY()
    TMap<FName, FIKRetargetPose> TargetRetargetPoses;
    UPROPERTY()
    FName CurrentSourceRetargetPose;
    UPROPERTY()
    FName CurrentTargetRetargetPose;
    //! UE 5.0 saves a single set of retarget poses for the target.
    UPROPERTY()
    TMap<FName, FIKRetargetPose> RetargetPoses;
    UPROPERTY()
    FName CurrentRetargetPose;

    explicit UIKRetargeter(const FObjectInitializer& ObjectInitializer);
};

}
