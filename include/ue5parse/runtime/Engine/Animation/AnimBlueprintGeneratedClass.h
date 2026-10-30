#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimClassInterface.h"
#include "ue5parse/runtime/Engine/Animation/AnimStateMachineTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeData.h"
#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"
#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"
#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "AnimBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class USkeleton;
struct FAnimNode_Base;

// The class of a cooked animation Blueprint (UE's Animation/AnimBlueprintGeneratedClass.h). The root class owns the
// graph: its node structs are properties of the instance. The nodes' folded properties are constants of the class's
// sparse class data or values of the instance's mutable data struct.
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
    // The notifies of the state machines' states and transitions.
    UPROPERTY()
    TArray<FAnimNotifyEvent> AnimNotifies;

    // The node lists follow from the class's properties (UE's Link). The link targets and the constant table need the
    // default object and its sparse class data (UE's PostLoadDefaultObject).
    void Link() override;
    void SerializeDefaultObject(UObject* Object, FArchive& Ar) override;

    const UAnimBlueprintGeneratedClass* GetRootAnimClass() const;
    USkeleton* GetTargetSkeleton() const { return TargetSkeleton; }
    // The node structs of an instance, in property order: a FPoseLink's LinkID indexes them.
    const TArray<FStructProperty*>& GetAnimNodeProperties() const { return AnimNodeProperties; }
    // The node a link to a node index runs: the node itself, or for a node type without a C++ implementation the
    // target of its first input link, which passes its pose through (INDEX_NONE for no input).
    int32 GetLinkTarget(int32 LinkID) const { return LinkTargets[LinkID]; }
    // The SaveCachedPose nodes of a graph in update order, as indices from the end of GetAnimNodeProperties.
    const TArray<int32>* GetOrderedSavedPoseNodeIndices(FName GraphName) const;
    // The index of a property of a node type in the node data's entries.
    int32 GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const;
    const void* GetConstantNodeValueRaw(int32 Index) const;
    const void* GetMutableNodeValueRaw(int32 Index, const UObject* Object) const;
    // The instance whose memory holds the node.
    const UObject* GetObjectPtrFromAnimNode(const FAnimNode_Base* Node) const;
    // The handler of the node of an index into the node data, or null when the class has none.
    const FExposedValueHandler* GetExposedValueHandler(int32 NodeIndex) const;
    const FPropertyAccessLibrary* GetPropertyAccessLibrary() const { return PropertyAccessLibrary; }
    // The class's graphs, the main AnimGraph first (UE's GenerateAnimationBlueprintFunctions).
    const TArray<FAnimBlueprintFunction>& GetAnimBlueprintFunctions() const { return AnimBlueprintFunctions; }
    const TMap<FName, FAnimGraphBlendOptions>& GetGraphBlendOptions() const { return GraphBlendOptions; }
    // The class whose node data the nodes use: this class, or for a child class without its own graph the nearest
    // parent with one.
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
