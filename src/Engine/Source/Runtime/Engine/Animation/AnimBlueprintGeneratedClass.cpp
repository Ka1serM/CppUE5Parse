#include "Engine/Source/Runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNode_LinkedInputPose.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNode_Root.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSubsystem_Base.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSubsystem_PropertyAccess.h"
#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"
#include "Engine/Source/Runtime/Engine/Animation/PropertyAccess.h"

namespace ue5parse::runtime {

namespace {

bool IsChildOfStruct(const UStruct* Struct, FName Name) {
    for (const UStruct* Type = Struct; Type != nullptr; Type = Type->GetSuperStruct())
        if (Type->GetFName() == Name)
            return true;
    return false;
}

template <class Visit>
void ForEachProperty(const UStruct* Struct, Visit&& InVisit) {
    for (const UStruct* Type = Struct; Type != nullptr; Type = Type->GetSuperStruct())
        for (FField* Field = Type->ChildProperties; Field != nullptr; Field = Field->Next)
            if (FProperty* Property = CastField<FProperty>(Field))
                InVisit(Property);
}

}

void UAnimBlueprintGeneratedClass::Link() {
    Super::Link();
    BuildNodeProperties();
}

void UAnimBlueprintGeneratedClass::SerializeDefaultObject(UObject* Object, FArchive& Ar) {
    Super::SerializeDefaultObject(Object, Ar);
    BuildLinkTargets(Object);
    BuildConstantProperties();
    GenerateAnimationBlueprintFunctions();
    LinkFunctionsToDefaultObjectNodes(Object);
}

const UAnimBlueprintGeneratedClass* UAnimBlueprintGeneratedClass::GetNodeDataClass() const {
    const UAnimBlueprintGeneratedClass* DataClass = this;
    while (DataClass->AnimNodeData.Num() == 0 && Cast<UAnimBlueprintGeneratedClass>(DataClass->GetSuperClass()))
        DataClass = Cast<UAnimBlueprintGeneratedClass>(DataClass->GetSuperClass());
    return DataClass;
}

void UAnimBlueprintGeneratedClass::GenerateAnimationBlueprintFunctions() {
    AnimBlueprintFunctions.Empty();
    const FName DefaultAnimGraphName("AnimGraph");
    for (const UStruct* Type = this; Type != nullptr; Type = Type->GetSuperStruct()) {
        for (UField* Field = Type->Children; Field != nullptr; Field = Field->Next) {
            const auto* Function = Cast<UFunction>(Field);
            if (Function == nullptr)
                continue;
            bool bFoundOutput = false;
            FStructProperty* OutputPoseNodeProperty = nullptr;
            TArray<FName> InputPoseNames;
            TArray<int32> InputPoseNodeIndices;
            TArray<FStructProperty*> InputPoseNodeProperties;
            TArray<FAnimBlueprintFunction::FInputPropertyData> InputPropertyData;
            for (FField* ParamField = Function->ChildProperties; ParamField != nullptr; ParamField = ParamField->Next) {
                FProperty* Param = CastField<FProperty>(ParamField);
                if (Param == nullptr)
                    continue;
                auto* StructProperty = CastField<FStructProperty>(Param);
                if (StructProperty == nullptr || !StructProperty->Struct->IsChildOf(FPoseLink::StaticStruct())) {
                    InputPropertyData.Add({Param->GetFName(), Param, nullptr});
                    continue;
                }
                if (StructProperty->HasAnyPropertyFlags(CPF_OutParm)) {
                    OutputPoseNodeProperty = bFoundOutput ? nullptr : StructProperty;
                    bFoundOutput = true;
                } else {
                    InputPoseNames.Add(StructProperty->GetFName());
                    InputPoseNodeIndices.Add(INDEX_NONE);
                    InputPoseNodeProperties.Add(nullptr);
                }
            }
            if (OutputPoseNodeProperty == nullptr)
                continue;
            const FName FunctionName = Function->GetFName();
            FAnimBlueprintFunction NewFunction(FunctionName);
            NewFunction.InputPoseNames = InputPoseNames;
            NewFunction.InputPoseNodeIndices = InputPoseNodeIndices;
            NewFunction.InputPoseNodeProperties = InputPoseNodeProperties;
            NewFunction.InputPropertyData = InputPropertyData;
            if (FunctionName == DefaultAnimGraphName)
                AnimBlueprintFunctions.Insert(NewFunction, 0);
            else
                AnimBlueprintFunctions.Add(NewFunction);
        }
    }
}

void UAnimBlueprintGeneratedClass::LinkFunctionsToDefaultObjectNodes(UObject* DefaultObject) {
    const UAnimBlueprintGeneratedClass* DataClass = GetNodeDataClass();
    auto FindFunction = [this](FName Name) -> FAnimBlueprintFunction* {
        for (FAnimBlueprintFunction& Function : AnimBlueprintFunctions)
            if (Function.Name == Name)
                return &Function;
        return nullptr;
    };
    for (int32 AnimNodeIndex = 0; AnimNodeIndex < AnimNodeProperties.Num(); ++AnimNodeIndex) {
        FStructProperty* StructProperty = AnimNodeProperties[AnimNodeIndex];
        if (LinkTargets[AnimNodeIndex] != AnimNodeIndex || !DataClass->AnimNodeData.IsValidIndex(AnimNodeIndex))
            continue;
        if (StructProperty->Struct->IsChildOf(FAnimNode_Root::StaticStruct())) {
            auto* RootNode = StructProperty->ContainerPtrToValuePtr<FAnimNode_Root>(DefaultObject);
            RootNode->SetNodeData(DataClass->AnimNodeData[AnimNodeIndex]);
            if (FAnimBlueprintFunction* FoundFunction = FindFunction(RootNode->GetName())) {
                FoundFunction->Group = RootNode->GetGroup();
                FoundFunction->OutputPoseNodeIndex = AnimNodeIndex;
                FoundFunction->OutputPoseNodeProperty = StructProperty;
            }
        } else if (StructProperty->Struct->IsChildOf(FAnimNode_LinkedInputPose::StaticStruct())) {
            auto* LinkedInputPoseNode = StructProperty->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(DefaultObject);
            FAnimBlueprintFunction* FoundFunction = FindFunction(LinkedInputPoseNode->Graph);
            if (FoundFunction == nullptr)
                continue;
            for (int32 InputIndex = 0; InputIndex < FoundFunction->InputPoseNames.Num(); ++InputIndex) {
                if (FoundFunction->InputPoseNames[InputIndex] != LinkedInputPoseNode->Name)
                    continue;
                FoundFunction->InputPoseNodeIndices[InputIndex] = AnimNodeIndex;
                FoundFunction->InputPoseNodeProperties[InputIndex] = StructProperty;
            }
            for (FAnimBlueprintFunction::FInputPropertyData& Input : FoundFunction->InputPropertyData)
                Input.ClassProperty = FindPropertyByName(Input.Name);
        }
    }
    for (FAnimBlueprintFunction& Function : AnimBlueprintFunctions) {
        if (Function.OutputPoseNodeProperty == nullptr)
            continue;
        const auto* RootNode = Function.OutputPoseNodeProperty->ContainerPtrToValuePtr<FAnimNode_Root>(DefaultObject);
        Function.bImplemented = RootNode->Result.LinkID != INDEX_NONE;
    }
}

const UAnimBlueprintGeneratedClass* UAnimBlueprintGeneratedClass::GetRootAnimClass() const {
    const UAnimBlueprintGeneratedClass* Root = this;
    while (const auto* Parent = Cast<UAnimBlueprintGeneratedClass>(Root->GetSuperClass()))
        Root = Parent;
    return Root;
}

void UAnimBlueprintGeneratedClass::BuildNodeProperties() {
    AnimNodeProperties.Empty();
    MutableProperties.Empty();
    MutableNodeDataProperty = nullptr;
    ForEachProperty(this, [this](FProperty* Property) {
        auto* StructProperty = CastField<FStructProperty>(Property);
        if (StructProperty == nullptr)
            return;
        if (IsChildOfStruct(StructProperty->Struct, FName("AnimNode_Base"))) {
            AnimNodeProperties.Add(StructProperty);
            return;
        }
        if (!IsChildOfStruct(StructProperty->Struct, FName("AnimBlueprintMutableData")))
            return;
        MutableNodeDataProperty = StructProperty;
        ForEachProperty(StructProperty->Struct, [this](FProperty* Mutable) { MutableProperties.Add(Mutable); });
    });
}

void UAnimBlueprintGeneratedClass::BuildLinkTargets(const UObject* DefaultObject) {
    LinkTargets.SetNum(AnimNodeProperties.Num());
    for (int32 NodeIndex = 0; NodeIndex < AnimNodeProperties.Num(); ++NodeIndex) {
        LinkTargets[NodeIndex] = ResolveLinkTarget(DefaultObject, NodeIndex, 0);
        if (LinkTargets[NodeIndex] != NodeIndex)
            ReportLoadProblem("anim.node_unsupported", GetName() + FString(": ") + AnimNodeProperties[NodeIndex]->Struct->GetName());
    }
}

int32 UAnimBlueprintGeneratedClass::ResolveLinkTarget(const UObject* DefaultObject, int32 NodeIndex, int32 Depth) const {
    if (NodeIndex == INDEX_NONE || Depth > AnimNodeProperties.Num())
        return INDEX_NONE;
    const FStructProperty* Node = AnimNodeProperties[NodeIndex];
    const auto* NodeType = Cast<UScriptStruct>(Node->Struct);
    if (NodeType->GetCppStructOps() != nullptr)
        return NodeIndex;
    const void* NodeData = Node->ContainerPtrToValuePtr<void>(DefaultObject);
    int32 InputLinkID = INDEX_NONE;
    ForEachProperty(NodeType, [&](FProperty* Property) {
        const auto* Link = CastField<FStructProperty>(Property);
        if (InputLinkID != INDEX_NONE || Link == nullptr || !IsChildOfStruct(Link->Struct, FName("PoseLinkBase")))
            return;
        const FProperty* LinkID = Link->Struct->FindPropertyByName(FName("LinkID"));
        InputLinkID = *LinkID->ContainerPtrToValuePtr<int32>(Link->ContainerPtrToValuePtr<void>(NodeData));
    });
    return ResolveLinkTarget(DefaultObject, InputLinkID, Depth + 1);
}

void UAnimBlueprintGeneratedClass::BuildConstantProperties() {
    ConstantProperties.Empty();
    BaseSubsystem = nullptr;
    SubsystemPropertyAccessLibrary = nullptr;
    if (SparseClassData == nullptr)
        return;
    TArray<FAnimSubsystem*> Subsystems;
    ForEachProperty(SparseClassDataStruct, [&](FProperty* Property) {
        ConstantProperties.Add(Property);
        const auto* StructProperty = CastField<FStructProperty>(Property);
        if (StructProperty == nullptr || !StructProperty->Struct->IsChildOf(FAnimSubsystem::StaticStruct()))
            return;
        auto* Subsystem = StructProperty->ContainerPtrToValuePtr<FAnimSubsystem>(SparseClassData);
        Subsystems.Add(Subsystem);
        if (StructProperty->Struct->IsChildOf(FAnimSubsystem_Base::StaticStruct()))
            BaseSubsystem = static_cast<FAnimSubsystem_Base*>(Subsystem);
        else if (StructProperty->Struct->IsChildOf(FAnimSubsystem_PropertyAccess::StaticStruct()))
            SubsystemPropertyAccessLibrary = &static_cast<FAnimSubsystem_PropertyAccess*>(Subsystem)->GetLibrary();
    });
    if (SubsystemPropertyAccessLibrary != nullptr)
        SubsystemPropertyAccessLibrary->PatchPropertyOffsets();
    for (FAnimSubsystem* Subsystem : Subsystems)
        Subsystem->OnPostLoadDefaults(this);
}

const TArray<int32>* UAnimBlueprintGeneratedClass::GetOrderedSavedPoseNodeIndices(FName GraphName) const {
    const FCachedPoseIndices* Indices = OrderedSavedPoseIndicesMap.Find(GraphName);
    return Indices ? &Indices->OrderedSavedPoseNodeIndices : nullptr;
}

int32 UAnimBlueprintGeneratedClass::GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const {
    const FAnimNodeStructData* StructData = NodeTypeMap.Find(NodeType);
    if (StructData == nullptr)
        return INDEX_NONE;
    const int32* Index = StructData->NameToIndexMap.Find(PropertyName);
    return Index ? *Index : INDEX_NONE;
}

const void* UAnimBlueprintGeneratedClass::GetConstantNodeValueRaw(int32 Index) const {
    return ConstantProperties[Index]->ContainerPtrToValuePtr<void>(GetSparseClassData());
}

const void* UAnimBlueprintGeneratedClass::GetMutableNodeValueRaw(int32 Index, const UObject* Object) const {
    const void* MutableData = MutableNodeDataProperty->ContainerPtrToValuePtr<void>(Object);
    return MutableProperties[Index]->ContainerPtrToValuePtr<void>(MutableData);
}

const UObject* UAnimBlueprintGeneratedClass::GetObjectPtrFromAnimNode(const FAnimNode_Base* Node) const {
    const FStructProperty* NodeProperty = AnimNodeProperties[Node->GetNodeData()->GetNodeIndex()];
    return reinterpret_cast<const UObject*>(reinterpret_cast<const uint8*>(Node) - NodeProperty->GetOffset_ForInternal());
}

const FExposedValueHandler* UAnimBlueprintGeneratedClass::GetExposedValueHandler(int32 NodeIndex) const {
    if (BaseSubsystem == nullptr)
        return nullptr;
    return &BaseSubsystem->GetExposedValueHandlers()[NodeIndex];
}

}
