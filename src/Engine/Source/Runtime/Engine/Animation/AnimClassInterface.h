#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AnimClassInterface.generated.h"

namespace ue5parse::runtime {

class FProperty;
class FStructProperty;
class UBlendProfile;

struct FAnimBlueprintFunction {
    explicit FAnimBlueprintFunction(const FName& InName) : Name(InName) {}

    struct FInputPropertyData {
        FName Name;
        FProperty* FunctionProperty = nullptr;
        FProperty* ClassProperty = nullptr;
    };

    FName Name;
    FName Group;
    int32 OutputPoseNodeIndex = INDEX_NONE;
    TArray<FName> InputPoseNames;
    TArray<int32> InputPoseNodeIndices;
    FStructProperty* OutputPoseNodeProperty = nullptr;
    TArray<FStructProperty*> InputPoseNodeProperties;
    TArray<FInputPropertyData> InputPropertyData;
    bool bImplemented = false;
};

USTRUCT()
struct FAnimGraphBlendOptions {
    GENERATED_BODY()
public:
    UPROPERTY()
    float BlendInTime = -1.0f;
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendInProfile = nullptr;
    UPROPERTY()
    float BlendOutTime = -1.0f;
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendOutProfile = nullptr;
};

}
