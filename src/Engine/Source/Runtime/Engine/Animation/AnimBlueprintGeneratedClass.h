#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimClassInterface.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimStateMachineTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeData.h"
#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"
#include "Engine/Source/Runtime/Engine/Animation/PropertyAccess.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "AnimBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class USkeleton;
struct FAnimNode_Base;

UCLASS()
class UAnimBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBakedAnimationStateMachine> BakedStateMachines;
    UPROPERTY()
    TArray<FAnimNodeData> AnimNodeData;
    UPROPERTY()
    TObjectPtr<USkeleton> TargetSkeleton = nullptr;
    UPROPERTY()
    TArray<FName> SyncGroupNames;
    UPROPERTY()
    TMap<FName, FAnimGraphBlendOptions> GraphBlendOptions;
    UPROPERTY()
    TArray<FAnimNotifyEvent> AnimNotifies;

    void Link() override;
    void SerializeDefaultObject(UObject* Object, FArchive& Ar) override;

    const UAnimBlueprintGeneratedClass* GetRootAnimClass() const;
    USkeleton* GetTargetSkeleton() const { return TargetSkeleton; }
    const TArray<FStructProperty*>& GetAnimNodeProperties() const { return AnimNodeProperties; }
    int32 GetLinkTarget(int32 LinkID) const { return LinkTargets[LinkID]; }
    const TArray<int32>* GetOrderedSavedPoseNodeIndices(FName GraphName) const;
    int32 GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const;
    const void* GetConstantNodeValueRaw(int32 Index) const;
    const void* GetMutableNodeValueRaw(int32 Index, const UObject* Object) const;
    const UObject* GetObjectPtrFromAnimNode(const FAnimNode_Base* Node) const;
    const FExposedValueHandler* GetExposedValueHandler(int32 NodeIndex) const;
    const FPropertyAccessLibrary* GetPropertyAccessLibrary() const { return PropertyAccessLibrary; }
    const TArray<FAnimBlueprintFunction>& GetAnimBlueprintFunctions() const { return AnimBlueprintFunctions; }
    const TMap<FName, FAnimGraphBlendOptions>& GetGraphBlendOptions() const { return GraphBlendOptions; }
    const UAnimBlueprintGeneratedClass* GetNodeDataClass() const;

private:
    void BuildNodeProperties();
    void BuildLinkTargets(const UObject* DefaultObject);
    int32 ResolveLinkTarget(const UObject* DefaultObject, int32 NodeIndex, int32 Depth) const;
    void BuildConstantProperties();
    void GenerateAnimationBlueprintFunctions();
    void LinkFunctionsToDefaultObjectNodes(UObject* DefaultObject);

    TArray<FStructProperty*> AnimNodeProperties;
    TArray<int32> LinkTargets;
    FStructProperty* MutableNodeDataProperty = nullptr;
    TArray<FProperty*> MutableProperties;
    TArray<FProperty*> ConstantProperties;
    const FArrayProperty* ExposedValueHandlersProperty = nullptr;
    void* ExposedValueHandlers = nullptr;
    FPropertyAccessLibrary* PropertyAccessLibrary = nullptr;
    TMap<FName, TArray<int32>> OrderedSavedPoseNodeIndices;
    TArray<FAnimBlueprintFunction> AnimBlueprintFunctions;
};

}
