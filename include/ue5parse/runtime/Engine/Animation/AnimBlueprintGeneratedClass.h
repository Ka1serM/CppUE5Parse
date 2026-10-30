#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimStateMachineTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeData.h"
#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"
#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"
#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "AnimBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class USkeleton;

// The class of a cooked animation Blueprint. The root class owns the graph; a child class supplies its own defaults.
UCLASS()
class UAnimBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBakedAnimationStateMachine> BakedStateMachines;
    UPROPERTY()
    TArray<FAnimNodeData> AnimNodeData;
    UPROPERTY()
    USkeleton* TargetSkeleton = nullptr;
    UPROPERTY()
    TArray<FName> SyncGroupNames;

    const UAnimBlueprintGeneratedClass* GetRootAnimClass() const;
    TArray<FStructProperty*> GetAnimNodeProperties() const;
    int32 GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const;
    int32 GetAnimNodePropertyCount(const UScriptStruct* NodeType) const;
    const void* GetAnimNodeValue(const FAnimNodeData& NodeData, const UScriptStruct* NodeType,
                                 FName PropertyName, const UObject* Instance, const FProperty*& ValueProperty) const;
    const void* GetAnimNodeFieldValue(int32 NodeIndex, FName PropertyName, const UObject* Instance,
                                      const FProperty*& ValueProperty) const;
    int32 GetAnimNodeLinkID(int32 NodeIndex, FName PropertyName, const UObject* Instance = nullptr) const;
    TArray<int32> GetAnimNodeLinkIDs(int32 NodeIndex, FName PropertyName, const UObject* Instance = nullptr) const;
    const FExposedValueHandler* GetExposedValueHandler(int32 NodeIndex) const;
    const FPropertyAccessLibrary* GetPropertyAccessLibrary() const;
};

}
