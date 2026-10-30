#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "AnimClassInterface.generated.h"

namespace ue5parse::runtime {

class FProperty;
class FStructProperty;
class UBlendProfile;

// A graph of an anim class that another graph can link to: its root and its input poses (UE's
// Animation/AnimClassInterface.h). The class makes one for each of its functions with an output pose.
struct FAnimBlueprintFunction {
    explicit FAnimBlueprintFunction(const FName& InName) : Name(InName) {}

    struct FInputPropertyData {
        FName Name;
        FProperty* FunctionProperty = nullptr;
        FProperty* ClassProperty = nullptr;
    };

    FName Name;
    FName Group;
    // The index of the root node in the class's node properties.
    int32 OutputPoseNodeIndex = INDEX_NONE;
    TArray<FName> InputPoseNames;
    TArray<int32> InputPoseNodeIndices;
    FStructProperty* OutputPoseNodeProperty = nullptr;
    TArray<FStructProperty*> InputPoseNodeProperties;
    TArray<FInputPropertyData> InputPropertyData;
    // Whether the graph has nodes: its root is linked.
    bool bImplemented = false;
};

// The inertial blends of a linked graph when it starts or stops being linked.
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
